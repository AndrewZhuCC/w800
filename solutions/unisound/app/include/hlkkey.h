
#include <stdint.h>


#define KEYIMG_LEN      32  
#define KEY_LEN         6  
#define HLKKEY_BASE     0x180ff000  

extern const unsigned char key1[];
extern const unsigned char key2[];
extern const unsigned char key3[];

int h_keyCheck(char *keyimg);
int h_setk(char const *k, char *keyimg);
int h_getm(char *mac);
int h_getk(char *k, char const *keyimg0);
int h_create_k(char *m, char *k);



unsigned int hex2i(const char *p);
int i2strhex(unsigned int i, char *str);
int hexstr2u8(char * buf,  char * hexs, int len);
unsigned char chk_sum_b(char * p, int len);
unsigned int chk_sum(unsigned char * p, int len);
