// j2kdll.h

#ifdef J2KDLLAPI
#else
#define J2KDLLAPI extern "C" __declspec(dllimport)
#endif

J2KDLLAPI int ImageTranscode(char source[], char target[], int ratio);


// end 