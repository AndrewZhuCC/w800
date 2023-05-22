typedef struct wnvcbuffer
{
	unsigned short length;
	const char *buffer;
} wnvcbuffer_t;

#define  WNVC_BUFF_TAB_NUM		129
typedef enum
{
	BEEP_000 = 0,BEEP_0001, BEEP_0002, BEEP_0003, 
	BEEP_0004, BEEP_0005, BEEP_0006, BEEP_0007, 
	BEEP_0008, BEEP_0009, BEEP_0010, BEEP_0011, 
	BEEP_0012, BEEP_0013, BEEP_0014, BEEP_0015, 
	BEEP_0016, BEEP_0017, BEEP_0018, BEEP_0019, 
	BEEP_0020, BEEP_0021, BEEP_0022, BEEP_0023, 
	BEEP_0024, BEEP_0025, BEEP_0026, BEEP_0027, 
	BEEP_0028, BEEP_0029, BEEP_0030, BEEP_0031, 
	BEEP_0032, BEEP_0033, BEEP_0034, BEEP_0035, 
	BEEP_0036, BEEP_0037, BEEP_0038, BEEP_0039, 
	BEEP_0040, BEEP_0041, BEEP_0042, BEEP_0043, 
	BEEP_0044, BEEP_0045, BEEP_0046, BEEP_0047, 
	BEEP_0048, BEEP_0049, BEEP_0050, BEEP_0051, 
	BEEP_0052, BEEP_0053, BEEP_0054, BEEP_0055, 
	BEEP_0056, BEEP_0057, BEEP_0058, BEEP_0059, 
	BEEP_0060, BEEP_0061, BEEP_0062, BEEP_0063, 
	BEEP_0064, BEEP_0065, BEEP_0066, BEEP_0067, 
	BEEP_0068, BEEP_0069, BEEP_0070, BEEP_0071, 
	BEEP_0072, BEEP_0073, BEEP_0074, BEEP_0075, 
	BEEP_0076, BEEP_0077, BEEP_0078, BEEP_0079, 
	BEEP_0080, BEEP_0081, BEEP_0082, BEEP_0083, 
	BEEP_0084, BEEP_0085, BEEP_0086, BEEP_0087, 
	BEEP_0088, BEEP_0089, BEEP_0090, BEEP_0091, 
	BEEP_0092, BEEP_0093, BEEP_0094, BEEP_0095, 
	BEEP_0096, BEEP_0097, BEEP_0098, BEEP_0099, 
	BEEP_0100, BEEP_0101, BEEP_0102, BEEP_0103, 
	BEEP_0104, BEEP_0105, BEEP_0106, BEEP_0107, 
	BEEP_0108, BEEP_0109, BEEP_0110, BEEP_0111, 
	BEEP_0112, BEEP_0113, BEEP_0114, BEEP_0115, 
	BEEP_0116, BEEP_0117, BEEP_0118, BEEP_0119, 
	BEEP_0120, BEEP_0121, BEEP_0122, BEEP_0123, 
	BEEP_0124, BEEP_0125, BEEP_0126, BEEP_0127, 
	BEEP_0128, BEEP_0129, 
}name;
extern wnvcbuffer_t wnvc_buffer_tab[WNVC_BUFF_TAB_NUM];
extern const char beep_0001[122 * 44];
extern const char beep_0002[41 * 44];
extern const char beep_0003[82 * 44];
extern const char beep_0004[65 * 44];
extern const char beep_0005[318 * 44];
extern const char beep_0006[64 * 44];
extern const char beep_0007[65 * 44];
extern const char beep_0008[60 * 44];
extern const char beep_0009[138 * 44];
extern const char beep_0010[135 * 44];
extern const char beep_0011[137 * 44];
extern const char beep_0012[131 * 44];
extern const char beep_0013[134 * 44];
extern const char beep_0014[68 * 44];
extern const char beep_0015[63 * 44];
extern const char beep_0016[66 * 44];
extern const char beep_0017[61 * 44];
extern const char beep_0018[62 * 44];
extern const char beep_0019[66 * 44];
extern const char beep_0020[63 * 44];
extern const char beep_0021[67 * 44];
extern const char beep_0022[63 * 44];
extern const char beep_0023[66 * 44];
extern const char beep_0024[57 * 44];
extern const char beep_0025[57 * 44];
extern const char beep_0026[52 * 44];
extern const char beep_0027[54 * 44];
extern const char beep_0028[52 * 44];
extern const char beep_0029[53 * 44];
extern const char beep_0030[51 * 44];
extern const char beep_0031[57 * 44];
extern const char beep_0032[57 * 44];
extern const char beep_0033[59 * 44];
extern const char beep_0034[59 * 44];
extern const char beep_0035[58 * 44];
extern const char beep_0036[56 * 44];
extern const char beep_0037[60 * 44];
extern const char beep_0038[57 * 44];
extern const char beep_0039[58 * 44];
extern const char beep_0040[54 * 44];
extern const char beep_0041[60 * 44];
extern const char beep_0042[61 * 44];
extern const char beep_0043[60 * 44];
extern const char beep_0044[61 * 44];
extern const char beep_0045[63 * 44];
extern const char beep_0046[59 * 44];
extern const char beep_0047[67 * 44];
extern const char beep_0048[61 * 44];
extern const char beep_0049[64 * 44];
extern const char beep_0050[61 * 44];
extern const char beep_0051[71 * 44];
extern const char beep_0052[75 * 44];
extern const char beep_0053[74 * 44];
extern const char beep_0054[73 * 44];
extern const char beep_0055[74 * 44];
extern const char beep_0056[71 * 44];
extern const char beep_0057[71 * 44];
extern const char beep_0058[68 * 44];
extern const char beep_0059[61 * 44];
extern const char beep_0060[62 * 44];
extern const char beep_0061[61 * 44];
extern const char beep_0062[63 * 44];
extern const char beep_0063[61 * 44];
extern const char beep_0064[63 * 44];
extern const char beep_0065[64 * 44];
extern const char beep_0066[62 * 44];
extern const char beep_0067[63 * 44];
extern const char beep_0068[62 * 44];
extern const char beep_0069[62 * 44];
extern const char beep_0070[65 * 44];
extern const char beep_0071[60 * 44];
extern const char beep_0072[66 * 44];
extern const char beep_0073[62 * 44];
extern const char beep_0074[85 * 44];
extern const char beep_0075[62 * 44];
extern const char beep_0076[63 * 44];
extern const char beep_0077[75 * 44];
extern const char beep_0078[77 * 44];
extern const char beep_0079[63 * 44];
extern const char beep_0080[62 * 44];
extern const char beep_0081[61 * 44];
extern const char beep_0082[64 * 44];
extern const char beep_0083[59 * 44];
extern const char beep_0084[62 * 44];
extern const char beep_0085[130 * 44];
extern const char beep_0086[128 * 44];
extern const char beep_0087[115 * 44];
extern const char beep_0088[116 * 44];
extern const char beep_0089[119 * 44];
extern const char beep_0090[134 * 44];
extern const char beep_0091[136 * 44];
extern const char beep_0092[151 * 44];
extern const char beep_0093[157 * 44];
extern const char beep_0094[148 * 44];
extern const char beep_0095[150 * 44];
extern const char beep_0096[147 * 44];
extern const char beep_0097[150 * 44];
extern const char beep_0098[138 * 44];
extern const char beep_0099[150 * 44];
extern const char beep_0100[153 * 44];
extern const char beep_0101[173 * 44];
extern const char beep_0102[71 * 44];
extern const char beep_0103[87 * 44];
extern const char beep_0104[74 * 44];
extern const char beep_0105[90 * 44];
extern const char beep_0106[90 * 44];
extern const char beep_0107[72 * 44];
extern const char beep_0108[62 * 44];
extern const char beep_0109[64 * 44];
extern const char beep_0110[62 * 44];
extern const char beep_0111[65 * 44];
extern const char beep_0112[66 * 44];
extern const char beep_0113[60 * 44];
extern const char beep_0114[63 * 44];
extern const char beep_0115[61 * 44];
extern const char beep_0116[63 * 44];
extern const char beep_0117[159 * 44];
extern const char beep_0118[58 * 44];
extern const char beep_0119[173 * 44];
extern const char beep_0120[206 * 44];
extern const char beep_0121[119 * 44];
extern const char beep_0122[67 * 44];
extern const char beep_0123[65 * 44];
extern const char beep_0124[75 * 44];
extern const char beep_0125[74 * 44];
extern const char beep_0126[75 * 44];
extern const char beep_0127[77 * 44];
extern const char beep_0128[74 * 44];
extern const char beep_0129[207 * 44];
