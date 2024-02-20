#define TEXT "SBCA0022"
__attribute__((__section__(".mb1text")))
char buf[10] = "1234567890";

int main()
{
    return buf[0];
}
