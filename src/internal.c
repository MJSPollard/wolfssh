/* internal.c 
 *
 * Copyright (C) 2014 wolfSSL Inc.
 *
 * This file is part of wolfSSH.
 *
 * wolfSSH is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfSSH is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */


#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include <wolfssh/ssh.h>
#include <wolfssh/internal.h>
#include <wolfssh/log.h>


typedef struct {
    uint8_t id;
    const char* name;
} NameIdPair;


static const NameIdPair NameIdMap[] = {
    { ID_NONE, "none" },
    { ID_AES128_CBC, "aes128-cbc" },
    { ID_AES128_CTR, "aes128-ctr" },
    { ID_AES128_GCM_WOLF, "aes128-gcm@wolfssl.com" },
    { ID_HMAC_SHA1, "hmac-sha1" },
    { ID_HMAC_SHA1_96, "hmac-sha1-96" },
    { ID_DH_GROUP1_SHA1, "diffie-hellman-group1-sha1" },
    { ID_DH_GROUP14_SHA1, "diffie-hellman-group14-sha1" },
    { ID_SSH_RSA, "ssh-rsa" }
};


uint8_t NameToId(const char* name, uint32_t nameSz)
{
    uint8_t id = ID_UNKNOWN;
    uint32_t i;
(void)nameSz;
    for (i = 0; i < (sizeof(NameIdMap)/sizeof(NameIdPair)); i++) {
        if (nameSz == WSTRLEN(NameIdMap[i].name) &&
            WSTRNCMP(name, NameIdMap[i].name, nameSz) == 0) {

            id = NameIdMap[i].id;
            break;
        }
    }

    return id;
}


const char* IdToName(uint8_t id)
{
    const char* name = NULL;
    uint32_t i;

    for (i = 0; i < (sizeof(NameIdMap)/sizeof(NameIdPair)); i++) {
        if (NameIdMap[i].id == id) {
            name = NameIdMap[i].name;
            break;
        }
    }

    return name;
}


Buffer* BufferNew(uint32_t size, void* heap)
{
    Buffer* newBuffer = NULL;

    if (size <= STATIC_BUFFER_LEN)
        size = STATIC_BUFFER_LEN;

    newBuffer = (Buffer*)WMALLOC(sizeof(Buffer), heap, WOLFSSH_TYPE_BUFFER);
    if (newBuffer != NULL) {
        WMEMSET(newBuffer, 0, sizeof(Buffer));
        newBuffer->heap = heap;
        newBuffer->bufferSz = size;
        if (size > STATIC_BUFFER_LEN) {
            newBuffer->buffer = (uint8_t*)WMALLOC(size,
                                                     heap, WOLFSSH_TYPE_BUFFER);
            if (newBuffer->buffer == NULL) {
                WFREE(newBuffer, heap, WOLFSSH_TYPE_BUFFER);
                newBuffer = NULL;
            }
            else
                newBuffer->dynamicFlag = 1;
        }
        else
            newBuffer->buffer = newBuffer->staticBuffer;
    }

    return newBuffer;
}


void BufferFree(Buffer* buf)
{
    if (buf != NULL) {
        if (buf->dynamicFlag)
            WFREE(buf->buffer, buf->heap, WOLFSSH_TYPE_BUFFER);
        WFREE(buf, bug->heap, WOLFSSH_TYPE_BUFFER);
    }
}


int GrowBuffer(Buffer* buf, uint32_t sz, uint32_t usedSz)
{
    WLOG(WS_LOG_DEBUG, "GB: buf = %p", buf);
    WLOG(WS_LOG_DEBUG, "GB: sz = %d", sz);
    WLOG(WS_LOG_DEBUG, "GB: usedSz = %d", usedSz);
    /* New buffer will end up being sz+usedSz long
     * empty space at the head of the buffer will be compressed */
    if (buf != NULL) {
        uint32_t newSz = sz + usedSz;
        WLOG(WS_LOG_DEBUG, "GB: newSz = %d", newSz);

        if (newSz > buf->bufferSz) {
            uint8_t* newBuffer = (uint8_t*)WMALLOC(newSz,
                                                buf->heap, WOLFSSH_TYPE_BUFFER);

            WLOG(WS_LOG_DEBUG, "Growing buffer");

            if (newBuffer == NULL)
                return WS_MEMORY_E;

            WLOG(WS_LOG_DEBUG, "GB: resizing buffer");
            if (buf->length > 0)
                WMEMCPY(newBuffer, buf->buffer + buf->idx, buf->length);

            if (!buf->dynamicFlag)
                buf->dynamicFlag = 1;
            else
                WFREE(buf->buffer, buf->heap, WOLFSSH_TYPE_BUFFER);

            buf->buffer = newBuffer;
            buf->bufferSz = newSz;
            buf->length = usedSz;
            buf->idx = 0;
        }
    }

    return WS_SUCCESS;
}


void ShrinkBuffer(Buffer* buf)
{
    if (buf != NULL) {
        uint32_t usedSz = buf->length - buf->idx;

        if (usedSz > STATIC_BUFFER_LEN)
            return;

        WLOG(WS_LOG_DEBUG, "Shrinking buffer");

        if (usedSz)
            WMEMCPY(buf->staticBuffer, buf->buffer + buf->idx, usedSz);

        WFREE(buf->buffer, buf->heap, WOLFSSH_TYPE_BUFFER);
        buf->dynamicFlag = 0;
        buf->buffer = buf->staticBuffer;
        buf->bufferSz = STATIC_BUFFER_LEN;
        buf->length = usedSz;
        buf->idx = 0;
    }
}

