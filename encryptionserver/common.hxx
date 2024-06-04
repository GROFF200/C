
#define ESC_NORMAL  0
#define ESC_SPACE   1

class escstring {
public:
    escstring();
    ~escstring();

    char *str_escape(unsigned char *s,int len = -1,int esc_flag = ESC_NORMAL);
    unsigned char *str_unescape(char *s,int *size);
private:
    int ahtoi(char c);
    char itoha(int i);

    char    *pesc;
};
