/*
 *  FIPS-197 compliant AES implementation
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS 
 *  The AES block cipher was designed by Vincent Rijmen and Joan Daemen.
 *
 *  http://csrc.nist.gov/encryption/aes/rijndael/Rijndael.pdf
 *  http://csrc.nist.gov/publications/fips/fips197/fips-197.pdf
 */



#include "oss_nv.h"
#include "xy_utils.h"
#include "factory_nv.h"
#include "cis_internals.h"
#if XY_DM
#include "cis_api.h"
#include "dm_endpoint.h"
#include "j_base64.h"
#include "cmcc_dm.h"
#include "mbedtls/sha256.h"
#include "mbedtls/aes.h"

#if _MSC_VER
#define snprintf _snprintf
#endif

extern cmcc_dm_regInfo_t *g_cmcc_dm_regInfo;
#if CIS_ENABLE_DM
#define DM_VER_DEFAULT                  "v2.0"
#define DM_APPKEY_DEFAULT               "M100000648"
#define DM_PWD_DEFAULT                  "x0g5g1q2LDm252162y2fZhgK8Cu8QZ7l"
#define DM_SN_DEFAULT                   "XY1200"
#define DM_IPADDRFAMLY_DEFAULT            4

void dmSdkInit(void *DMconfig)
{
	//to init g_opt
	DMOptions *g_opt=(DMOptions *)DMconfig;
	strcpy(g_opt->szDMv, DM_VER_DEFAULT);
    g_opt->nAddressFamily = DM_IPADDRFAMLY_DEFAULT;
    xy_get_IMEI(g_opt->szCMEI_IMEI, sizeof(g_opt->szCMEI_IMEI));          //模组IMEI
    xy_get_IMSI(g_opt->szIMSI, sizeof(g_opt->szIMSI));
    if(cissys_getSN(g_opt->szSN,sizeof(g_opt->szSN)) <= 0)
        strcpy(g_opt->szSN, DM_SN_DEFAULT);

    if(strlen((const char *)g_cmcc_dm_regInfo->dm_app_key) > 0)
        cis_memcpy(g_opt->szAppKey, g_cmcc_dm_regInfo->dm_app_key, sizeof(g_cmcc_dm_regInfo->dm_app_key));
    else
        strcpy(g_opt->szAppKey, DM_APPKEY_DEFAULT);

    if(strlen((const char *)g_cmcc_dm_regInfo->dm_app_pwd) > 0)
        cis_memcpy(g_opt->szPwd, g_cmcc_dm_regInfo->dm_app_pwd, sizeof(g_cmcc_dm_regInfo->dm_app_pwd));
    else
        strcpy(g_opt->szPwd, DM_PWD_DEFAULT);

	return ;
}
#endif
#define EP_MEM_SIZE  (264)
#define AES_BLOCK_SIZE 16

int my_aes_encrypt(char* enckey,char* encbuf, char* decbuf,int inlen,int* outlen)

{

		char key[34]="";// = "12345678"

		unsigned char iv[16] = "";

		cis_memset(key,0,sizeof(key));	
		cis_memset(iv,0,sizeof(iv));

		cis_memcpy(key, enckey, sizeof(key));

	    int nLen = inlen;//input_string.length();
	    int nBei;
		if((!encbuf)||(!decbuf))

			return -1;
		nBei = nLen / AES_BLOCK_SIZE + 1;
    int nTotal = nBei * AES_BLOCK_SIZE;


	    unsigned char *enc_s = ( unsigned char*)cis_malloc(nTotal);

	    #ifdef DEBUG_INFO

        xy_printf(0,XYAPP, WARN_LOG, "de enc_s=%p,size=%d,len=%d\n",enc_s,nTotal,nLen);

        #endif

	    if(enc_s==NULL)

	    {

			xy_printf(0,XYAPP, WARN_LOG, "enc_s mem err\n");

			return -1;

		}

	    int nNumber;

	    if (nLen % AES_BLOCK_SIZE > 0)

	        nNumber = nTotal - nLen;

	    else

	        nNumber = AES_BLOCK_SIZE;

	    cis_memset(enc_s, nNumber, nTotal);

	    cis_memcpy(enc_s, encbuf, nLen);

	    mbedtls_aes_context ctx;
	    mbedtls_aes_init( &ctx );
	    mbedtls_aes_setkey_enc( &ctx, (const unsigned char *)key, 256);
        mbedtls_aes_crypt_cbc( &ctx, MBEDTLS_AES_ENCRYPT, nBei * AES_BLOCK_SIZE, iv, enc_s, (unsigned char *)decbuf );
	    mbedtls_aes_free( &ctx );

		*outlen = nBei * AES_BLOCK_SIZE;

		cis_free(enc_s);

		enc_s=NULL;

	return 0;

}

void my_sha256(const char *src,int srclen, char *resultT )
{ 
	mbedtls_sha256( (const unsigned char*)src,srclen, (unsigned char*)resultT,0);
	return ;
}
#if CIS_ENABLE_DM
int genDmRegEndpointName(char ** data,void *dmconfig)
{	

	 char key[64]={0};
	 char *szEpname=NULL;
	 char *ciphertext=NULL;   
	 char *szStars="****";
	 char * name = "";
	int  ciphertext_len,ret=0;
	 char *base64tmp=NULL;
	 char *epnametmp=NULL;
	int i=0;
	 char *passwd ="00000000000000000000000000000000";
	unsigned char *encData=0;
	//unsigned char *decData=0;
	int encDataLen=0;
	//int decDataLen=0;
	/* Buffer for the decrypted text */  
   char *plaintext =  "plaintext";
	//base64
   char *testbase64="123456789";//MTIzNDU2Nzg5
	 DMOptions *g_opt=(DMOptions *)dmconfig;

  szEpname=( char *)cis_malloc(EP_MEM_SIZE);
  if(szEpname==NULL)
  {
		xy_printf(0,XYAPP, WARN_LOG, "mem err r1\n");ret=-1;
		goto out;
	}
	ciphertext=( char *)cis_malloc(EP_MEM_SIZE);
  if(ciphertext==NULL)
  {
		xy_printf(0,XYAPP, WARN_LOG, "mem err r2\n");ret=-1;
		goto out;
	}

	cis_memset(ciphertext,0,EP_MEM_SIZE);
    cis_memset(szEpname,0,EP_MEM_SIZE);
	sprintf(szEpname,"%s-%s-%s-%s",
	strlen((const char *)(g_opt->szCMEI_IMEI))>0?g_opt->szCMEI_IMEI:szStars,
	strlen((const char *)(g_opt->szCMEI_IMEI2))>0?g_opt->szCMEI_IMEI2:szStars,
	strlen((const char *)(g_opt->szIMSI))>0?g_opt->szIMSI:szStars,	
	strlen((const char *)(g_opt->szDMv))>0?g_opt->szDMv:szStars);
	//xy_printf(0,XYAPP, WARN_LOG, "reg szEpname:%s,%d\n",szEpname,strlen(szEpname));
	if (strlen((const char *)name)<=0)
		name=szEpname;
	if(strlen((const char *)(g_opt->szPwd))>0)
	{
		passwd = g_opt->szPwd;
	}
	else
	{
		xy_printf(0,XYAPP, WARN_LOG, "pwd is null,use default pwd is:%s~\n", passwd);
	}

	my_sha256(passwd,strlen((const char *)passwd),key);

	/* A 128 bit IV */  

	plaintext = name;
	/* Encrypt the plaintext */  
  my_aes_encrypt((char *)key,(char *)plaintext, ciphertext,strlen((const char *)plaintext),&ciphertext_len );  

	name = ciphertext; //???????

	testbase64=name;
	base64tmp=( char *)cis_malloc(EP_MEM_SIZE);//szEpname is free now,use again;

	if(base64tmp==NULL)
  {
		xy_printf(0,XYAPP, WARN_LOG, "mem err r4\n");ret=-1;
		goto out;
	}
	cis_memset(base64tmp,0,EP_MEM_SIZE);

	ret= j_base64_encode((unsigned char *)testbase64, ciphertext_len, &encData, (unsigned int *)&encDataLen);
	cis_memcpy(base64tmp,encData,encDataLen);
  j_base64_free(encData, encDataLen);
 	epnametmp=( char *)cis_malloc(EP_MEM_SIZE*4);
	if(epnametmp==NULL)
  {
		xy_printf(0,XYAPP, WARN_LOG, "mem err\n,3");ret=-1;
		goto out;
	}
	cis_memset(epnametmp,0,EP_MEM_SIZE*4);
	snprintf((char *)epnametmp,EP_MEM_SIZE*4,"I@#@%s@#@%s@#@%s@#@%s@#@%s+%s@#@%s@#@%d",base64tmp,g_opt->szAppKey,
		strlen((const char *)(g_opt->szCMEI_IMEI))>0?g_opt->szCMEI_IMEI:szStars,
		strlen((const char *)(g_opt->szDMv))>0?g_opt->szDMv:szStars,
		g_opt->szAppKey,
		strlen((const char *)(g_opt->szCMEI_IMEI))>0?g_opt->szCMEI_IMEI:g_opt->szSN,
		g_opt->szSN,
		0);
	//xy_printf(0,XYAPP, WARN_LOG, "reg epname=%s,%d\n", epnametmp,strlen(epnametmp));
	name = epnametmp; //?????????
	///////////////
	*data=(char *)cis_malloc(strlen((const char *)name)+1);
	if(*data==NULL)
	{
			ret=-1;
			goto out;
	}
	cis_memset(*data,0,strlen((const char *)name)+1);
	cis_memcpy(*data,name,strlen((const char *)name));
	ret=0;
	out:
	if(szEpname)
	{
		cis_free(szEpname);szEpname=NULL;
	}
	if(ciphertext)
	{
		cis_free(ciphertext);ciphertext=NULL;
	}
	if(epnametmp)
	{
		cis_free(epnametmp);epnametmp=NULL;
	}
	if(base64tmp)
	{
		cis_free(base64tmp);base64tmp=NULL;
	}
	return ret;
}

int genDmUpdateEndpointName(char **data,void *dmconfig)
{
	#define EP_MEM_SIZE  (264)
	 char key[64]={0};
	 char *szEpname=NULL;
	 char *ciphertext=NULL;   
	 char *szStars="****";
     char * name = "";
     int  ciphertext_len;
	 char *base64tmp=NULL;
	 char *epnametmp=NULL;
	 int i=0,ret=-1;
	 char *passwd = "00000000000000000000000000000000";
	unsigned char *encData=0;
	//unsigned char *decData=0;
	int encDataLen=0;
	//int decDataLen=0;
	/* A 128 bit IV */  
	/* Buffer for the decrypted text */  
    char *plaintext =  "plaintext";
	 char *testbase64="123456789";//MTIzNDU2Nzg5
	 DMOptions *g_opt=(DMOptions *)dmconfig;

  szEpname=( char *)cis_malloc(EP_MEM_SIZE);
  if(szEpname==NULL)
  {
		xy_printf(0,XYAPP, WARN_LOG, "mem err u1\n");ret=-1;
		goto out;
	}
	ciphertext=( char *)cis_malloc(EP_MEM_SIZE);
  if(ciphertext==NULL)
  {
		xy_printf(0,XYAPP, WARN_LOG, "mem err u2\n");ret=-1;
		goto out;
	}
	cis_memset(ciphertext,0,EP_MEM_SIZE);
    cis_memset(szEpname,0,EP_MEM_SIZE);
	snprintf((char *)szEpname,EP_MEM_SIZE,"%s-%s-%s-%s",
	strlen((const char *)(g_opt->szCMEI_IMEI))>0?g_opt->szCMEI_IMEI:szStars,
	strlen((const char *)(g_opt->szCMEI_IMEI2))>0?g_opt->szCMEI_IMEI2:szStars,
	strlen((const char *)(g_opt->szIMSI))>0?g_opt->szIMSI:szStars,szStars);
	//xy_printf(0,XYAPP, WARN_LOG, "update szEpname:%s,%d\n",szEpname,strlen(szEpname));
	if (strlen((const char *)name)<=0)
		name=szEpname;

	if(strlen((const char *)(g_opt->szPwd))>0)
	{
		passwd = g_opt->szPwd;
	}
	else
	{
		xy_printf(0,XYAPP, WARN_LOG, "pwd is null,use default pwd is:%s~\n", passwd);
	}
	//sha256(passwd,strlen(passwd),key);

	my_sha256(passwd,strlen((const char *)passwd),key);


	plaintext = name;
	/* Encrypt the plaintext */  
	my_aes_encrypt((char *)key,plaintext, ciphertext,strlen((const char *)plaintext),&ciphertext_len );  
	name = ciphertext; //???????
	//base64

	testbase64=name;
	base64tmp=( char *)cis_malloc(EP_MEM_SIZE);//szEpname is free now,use again;
	if(base64tmp==NULL)
  {
		xy_printf(0,XYAPP, WARN_LOG, "mem err u4\n");
		goto out;
	}
	cis_memset(base64tmp,0,EP_MEM_SIZE);

	ret= j_base64_encode((unsigned char *)testbase64, ciphertext_len, &encData,( unsigned int *)&encDataLen);
	cis_memcpy(base64tmp,encData,encDataLen);	
    j_base64_free(encData, encDataLen);
	epnametmp=(char *)cis_malloc(EP_MEM_SIZE*4);
	if(epnametmp==NULL)
  {
		xy_printf(0,XYAPP, WARN_LOG, "mem err u3\n");ret=-1;
		goto out;
	}
	cis_memset(epnametmp,0,EP_MEM_SIZE*4);
	sprintf((char *)epnametmp,"I@#@%s@#@%s@#@%s@#@%s@#@%s+%s@#@%s@#@%d",base64tmp,g_opt->szAppKey,
		strlen((const char *)(g_opt->szCMEI_IMEI))>0?g_opt->szCMEI_IMEI:szStars,
		strlen((const char *)(g_opt->szDMv))>0?g_opt->szDMv:szStars,
		g_opt->szAppKey,
		strlen((const char *)(g_opt->szCMEI_IMEI))>0?g_opt->szCMEI_IMEI:g_opt->szSN,
		g_opt->szSN,
		0);
	//xy_printf(0,XYAPP, WARN_LOG, "update epname=%s,%d\n", epnametmp,strlen(epnametmp));
	name = epnametmp;
  *data=(char *)cis_malloc(strlen((const char *)name)+1);
	if(*data==NULL)
	{
			ret=-1;
			goto out;
	}
	cis_memset(*data,0,strlen((const char *)name)+1);
	cis_memcpy(*data,name,strlen((const char *)name));
    ret=0;
	out:
	if(szEpname)
	{
		cis_free(szEpname);szEpname=NULL;
	}
	if(ciphertext)
	{
		cis_free(ciphertext);ciphertext=NULL;
	}
	if(epnametmp)
	{
		cis_free(epnametmp);epnametmp=NULL;
	}
	if(base64tmp)
	{
		cis_free(base64tmp);base64tmp=NULL;
	}
	return ret;
}

int prv_getDmUpdateQueryLength(st_context_t * contextP,
                                          st_server_t * server)
{
    int index;
    //int res;
    //char buffer[21];

    (void) server;

    index = strlen("epi=");
    index += strlen((const char *)(contextP->DMprivData));
    return index + 1;
}

int prv_getDmUpdateQuery(st_context_t * contextP,
                                    st_server_t * server,
                                    uint8_t * buffer,
                                    size_t length)
{
    int index;
    int res/*,name_len*/;

    (void) server;

    index = utils_stringCopy((char *)buffer, length, "epi=");
    if (index < 0) return 0;
    res = utils_stringCopy((char *)buffer + index, length - index, (const char *)(contextP->DMprivData));
    if (res < 0) return 0;
    index += res;

    if(index < (int)length)
    {
        buffer[index++] = '\0';
    }
    else
    {
        return 0;
    }

    return index;
}
#endif
#endif
