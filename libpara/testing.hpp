#define _S(x) #x
#define _S_(x) _S(x)
#define _S__LINE__ _S_(__LINE__)

#define Expect(x) expect(x, "expected " #x " to be true", __FILE__, _S__LINE__)
