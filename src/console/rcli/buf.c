#include <string.h>

#include "stm32f10x.h"
#include "buf.h"

char buf_init(ctrlBuf_s *bufStruct, char *pBuf, unsigned char max_size)
{
    char res   = 0;

    if (bufStruct == NULL)
    {
        return 1;
    }

    bufStruct->cur_pos  = 1;
    bufStruct->end      = 0;
    bufStruct->is_clear = 1;
    bufStruct->pBuf     = pBuf;
    bufStruct->max_size = max_size;

    return res;
}

char buf_add(ctrlBuf_s *bufStruct, char symbol, unsigned char pos)
{
    char res = 0;

    if (bufStruct == NULL)
    {
        return 1;
    }

    if (bufStruct->end >= bufStruct->max_size)
    {
        return 1;
    }

    unsigned char end    = bufStruct->end;
    char *pBuf  = bufStruct->pBuf;
    char *pDest = NULL;
    char *pSrc  = NULL;
    unsigned char len    = 0;

    if (pos > end + 1)
    {
        pos = end + 1;
    }

    if (pos == 0)
    {
        pos = bufStruct->cur_pos;
    }

    if (pos - 1 >= 0)
    {
        len = pos - 1;
    }

    if (len > 0)
    {
        pSrc  = pBuf + sizeof(char) * (end - pos + 1);
        pDest = pSrc + sizeof(char);
        memcpy(pDest, pSrc, len);
    }
    pBuf[end - len] = symbol;

    bufStruct->end     += 1;
    bufStruct->is_clear = 0;
    bufStruct->cur_pos  = pos;

    return res;
}

char * buf_get_buf(ctrlBuf_s bufStruct)
{
    return bufStruct.pBuf;
}

char buf_move_cur(ctrlBuf_s *bufStruct, dirBuf_e dir)
{
    char res = 0;
    unsigned char pos;

    if (bufStruct == NULL)
    {
        return 1;
    }

    pos = bufStruct->cur_pos;

    switch(dir)
    {
        case BUF_CUR_LEFT:
        {
            if (pos - 1 < bufStruct->end)
            {
                pos += 1;
            }
            break;
        }

        case BUF_CUR_RIGHT:
        {
            if (pos > 1)
            {
                pos -= 1;
            }
            break;
        }

        case BUF_CUR_HOME:
        {
            pos = bufStruct->end + 1;
            break;
        }

        case BUF_CUR_END:
        {
            pos = 1;
            break;
        }

        default:
        {
            return 1;
        }
    }

    bufStruct->cur_pos = pos;
    return res;
}

char buf_del(ctrlBuf_s *bufStruct)
{
    unsigned char pos;
    unsigned char end;
    char *pBuf;
    char *pDest = NULL;
    char *pSrc  = NULL;
    unsigned char len    = 0;
    unsigned char res    = 0;

    if (bufStruct == NULL)
    {
        return 1;
    }

    pos  = bufStruct->cur_pos;
    end  = bufStruct->end;
    pBuf = bufStruct->pBuf;

    if (pos <= 1)
    {
        return res;
    }

    if (pos > 2)
    {
        len   = pos - 2;
        pDest = pBuf + sizeof(char) * (end - pos + 1);
        pSrc  = pDest + sizeof(char);
        memcpy(pDest, pSrc, len);
    }

    bufStruct->pBuf[end - 1] = 0;
    bufStruct->end -= 1;
    bufStruct->cur_pos -= 1;
    
    if (bufStruct->end == 0)
    {
        bufStruct->is_clear = 1;
    }

    return res;
}

char buf_get_count_params(ctrlBuf_s bufStruct)
{
    char * pBuf;
    char count = 0;

    pBuf = bufStruct.pBuf;

    if (pBuf == NULL)
    {
        return -1;
    }

    do
    {
        while (*pBuf == 32 && *pBuf++);
        if (*pBuf > 32 && *pBuf <= 126 )
        {
            count++;
            while (*pBuf > 32 && *pBuf <= 126 && *pBuf++);
        }
    }
    while(*pBuf++ != '\0');

    return count;
}

char buf_get_pos_n_word(ctrlBuf_s bufStruct, unsigned char n)
{
    char * pBuf;
    char pos = -1;
    char ret_val = -1;

    pBuf = bufStruct.pBuf;

    if (pBuf == NULL)
    {
        return -1;
    }

    do
    {
        while (*pBuf == 32 && *pBuf++)
        {
            pos++;
        }

        if (*pBuf >= 32 && *pBuf <= 126 )
        {
            pos++;
            ret_val = pos;
            while (*pBuf > 32 && *pBuf <= 126 && *pBuf++)
            {
                pos++; 
            }
        }
    }
    while(*pBuf++ != '\0' && n-- !=0);

    return ret_val;    
}

char buf_cpy_str(ctrlBuf_s *bufStruct, char *str, unsigned char len)
{
    if (bufStruct == NULL)
    {
        return -1;
    }

    buf_clear(bufStruct);
    strncpy(bufStruct->pBuf, str, len);
    bufStruct->cur_pos = 1;
    bufStruct->end = len;
    bufStruct->is_clear = 0;

    return 0;
}

char buf_clear(ctrlBuf_s *bufStruct)
{
    char res = 0;

    if (bufStruct == NULL)
    {
        return 1;
    }

    /* Param size in bytes. */
    memset(bufStruct->pBuf, '\0', bufStruct->max_size);

    bufStruct->is_clear = 1;
    bufStruct->cur_pos  = 1;
    bufStruct->end      = 0;

    return res;
}

char *buf_get_buf_from_pos(ctrlBuf_s bufStruct)
{
    char p = bufStruct.end + 1 - bufStruct.cur_pos;

    if (p < 0)
        return NULL;

    return (char*)(bufStruct.pBuf + p);
}

void buf_debug(ctrlBuf_s bufStruct)
{
    // printf("Debug:\r\n");
    // printf("\t cur_pos: %d\r\n", bufStruct.cur_pos);
    // printf("\t end: %d\r\n", bufStruct.end);
    // printf("\t is_clear: %d\r\n", bufStruct.is_clear);
    // printf("\t pBuf: %p\r\n", bufStruct.pBuf);
    // printf("\t max_size: %d\r\n", bufStruct.max_size);
    // printf("\t buf=\"%s\"\r\n", bufStruct.pBuf);
}
