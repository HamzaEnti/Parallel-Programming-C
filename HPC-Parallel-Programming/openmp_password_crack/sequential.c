#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <omp.h>
#include <openssl/sha.h>

// hex_to_bytes()
// Converteix una cadena hexadecimal (ex. "9f86d081...") a bytes binaris.
//  - hex: text amb caràcters hexadecimals
//  - out: buffer de sortida on es guarden els bytes
//  - outlen: nombre de bytes esperats (la meitat de la longitud del text)
// Retorna 0 si tot és correcte o -1 si el format no és vàlid.
int hex_to_bytes(const char *hex, unsigned char *out, size_t outlen){
    size_t len = strlen(hex);
    if(len != outlen*2) return -1;
    for(size_t i=0;i<outlen;i++){
        char a = hex[2*i], b = hex[2*i+1];
        if(!isxdigit((unsigned char)a) || !isxdigit((unsigned char)b)) return -1;
        int va = (a>='0' && a<='9') ? a-'0' : (tolower(a)-'a'+10);
        int vb = (b>='0' && b<='9') ? b-'0' : (tolower(b)-'a'+10);
        out[i] = (unsigned char)((va<<4)|vb);
    }
    return 0;
}

// seed_to_password()
// Converteix un número (seed) en una contrasenya de longitud fixa.
//  - seed: valor numèric a convertir
//  - buf: buffer on es desa la contrasenya generada (acaba amb '\0')
//  - password_len: longitud de la contrasenya desitjada
// Utilitza caràcters 'a'-'z' i '0'-'9' (base36) per formar la paraula.
void seed_to_password(uint32_t seed, char *buf, size_t password_len){
    const char *alphabet = "abcdefghijklmnopqrstuvwxyz0123456789";
    const size_t A = 36;
    /* generem en ordre little-endian i després invertim */
    char tmp[64];
    size_t i = 0;
    uint32_t v = seed;
    if(v == 0) tmp[i++] = alphabet[0];
    while(v > 0 && i < sizeof(tmp)){
        tmp[i++] = alphabet[v % A];
        v /= A;
    }
    /* si curta, omplim amb 'a' */
    while(i < password_len) tmp[i++] = alphabet[0];
    /* invertir a buf */
    for(size_t j=0;j<password_len;j++){
        buf[j] = tmp[password_len - 1 - j];
    }
    buf[password_len] = '\0';
}

// compute_sha256()
// Calcula el hash SHA-256 del bloc de dades indicat.
//  - data: punter a les dades d'entrada
//  - len: mida de les dades
//  - digest: buffer on es desa el resultat (32 bytes)
// Utilitza la llibreria OpenSSL per fer el càlcul.
static inline void compute_sha256(const unsigned char *data, size_t len, unsigned char *digest){
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data, len);
    SHA256_Final(digest, &ctx);
}

// parse_args()
// Llegeix i valida els paràmetres de línia d'ordres:
//   1) maxseed  -> nombre màxim d'iteracions a provar
//   2) pwdlen   -> longitud de la contrasenya
//   3) target   -> hash SHA-256 (64 caràcters hex) que volem trobar
// Desa els valors validats als punters out_* i retorna 0 si tot és correcte.
int parse_args(int argc, char **argv, uint32_t *out_maxseed, size_t *out_pwdlen, unsigned char *out_target){
    if(argc != 4){
        fprintf(stderr, "Ús: %s <maxseed> <pwdlen> <TARGET_SHA256_HEX>\n", argv[0]);
        return 1;
    }
    *out_maxseed = (uint32_t)strtoul(argv[1], NULL, 0);
    int t = atoi(argv[2]);
    if(t <= 0 || t >= 64){ fprintf(stderr, "pwdlen ha de ser entre 1 i 63\n"); return 2; }
    *out_pwdlen = (size_t)t;
    if(hex_to_bytes(argv[3], out_target, SHA256_DIGEST_LENGTH) != 0){
        fprintf(stderr, "TARGET_SHA256_HEX ha de ser 64 hex chars (32 bytes)\n"); return 3;
    }
    return 0;
}

int main(int argc, char **argv){
    uint32_t maxseed; // el nombre d'iteracions que volem provar
    size_t pwdlen; // mida de la contrasenya
    unsigned char target[SHA256_DIGEST_LENGTH]; // hash de la contrasenya (contrasenya xifrada)

    int rc = parse_args(argc, argv, &maxseed, &pwdlen, target);
    if(rc != 0) return rc;

    char local_pwd[64];
    unsigned char local_digest[SHA256_DIGEST_LENGTH];
    int found = 0;

    /* temps: inici del cronòmetre */
    double t0 = omp_get_wtime(); /* temps */

    /* Bucle per provar tots els seeds fins a maxseed */
    for(uint32_t i = 0; i < maxseed; ++i){

        if (found) continue;  /* sortida anticipada si ja s'ha trobat en un altre fil */

        char local_pwd_thr[64];
        unsigned char local_digest_thr[SHA256_DIGEST_LENGTH];

        /* genera la contrasenya per a aquest candidat 's' */
        seed_to_password(i, local_pwd, pwdlen);

        /* calcula SHA256 del candidat */
        compute_sha256((unsigned char*)local_pwd, pwdlen, local_digest);

        /* compara amb el target passat com a argument */
        if(memcmp(local_digest, target, SHA256_DIGEST_LENGTH) == 0){
            /* si coincideixen: hem trobat la contrasenya per a aquest seed */
            if (!found) {
                double telapsed_found = omp_get_wtime() - t0; /* temps */
                double tries = (double)i + 1.0;
                double rate = (telapsed_found > 0.0) ? (tries / telapsed_found) : 0.0; /* temps */
                printf("Coincideix! contrasenya='%s' per a seed=%u\n", local_pwd, i);
                printf("[temps] trobat en %.6f s (~%.0f intents/s, %u intents)\n",
                       telapsed_found, rate, (unsigned)i); /* temps */
                found = 1;
            }
        }
    }

    double ttotal = omp_get_wtime() - t0; /* temps */

    if(!found){
        printf("No s'ha trobat cap coincidència dins de maxseed=%u\n", maxseed);
        printf("[temps] escaneig total: %.6f s (~%.0f intents/s)\n",
               ttotal, (ttotal>0.0)? ((double)maxseed/ttotal) : 0.0); /* temps */
    }
    else {
        printf("[temps] temps total d'execució: %.6f s\n", ttotal); /* temps */
    }

    return 0;
}
