#ifndef BUF_H
#define BUF_H

typedef struct
{
    unsigned char cur_pos;
    unsigned char end;
    char is_clear;
    char* pBuf;
    unsigned char max_size;
} ctrlBuf_s;

typedef enum
{
    BUF_CUR_LEFT,
    BUF_CUR_RIGHT,
    BUF_CUR_HOME,
    BUF_CUR_END,
} dirBuf_e;

char buf_init(ctrlBuf_s *bufStruct, char *pBuf, unsigned char max_size);
char buf_add(ctrlBuf_s *bufStruct, char symbol, unsigned char pos);
char * buf_get_buf(ctrlBuf_s bufStruct);
char buf_move_cur(ctrlBuf_s *bufStruct, dirBuf_e dir);
char buf_del(ctrlBuf_s *bufStruct);
char buf_get_count_params(ctrlBuf_s bufStruct);
char buf_get_pos_n_word(ctrlBuf_s bufStruct, unsigned char n);
char buf_cpy_str(ctrlBuf_s *bufStruct, char *str, unsigned char len);
char * buf_get_buf_from_pos(ctrlBuf_s bufStruct);
char buf_clear(ctrlBuf_s *bufStruct);
void buf_debug(ctrlBuf_s bufStruct);

#endif
