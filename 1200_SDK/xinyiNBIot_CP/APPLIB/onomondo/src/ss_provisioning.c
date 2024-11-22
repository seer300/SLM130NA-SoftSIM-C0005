/** 
* @file     ss_provision.c
* @date     2024-06-25
* @author   Onomondo
* @brief    onomondo-uicc profile provisioning utility
*/

#include "onomondo/softsim/fs.h"
#include "onomondo/softsim/mem.h"
#include "onomondo/softsim/utils.h"
#include "onomondo/utils/ss_profile.h"
#include "onomondo/utils/ss_provisioning.h"

// Helper functions for profile decoding
uint8_t ss_hex_to_uint8(const char *hex);
void ss_hex_string_to_bytes(const uint8_t *hex, size_t hex_len, uint8_t bytes[hex_len / 2]);
uint8_t write_profile_to_littlefs(const struct ss_profile *profile);
int softsim_get_key(char *secretkey, int size);

/** Hex to uint8 converter
 *  \param[in] hex a pointer to hex value to be converted
 *  \returns converted uint8 value */
uint8_t ss_hex_to_uint8(const char *hex)
{
	char hex_str[3] = { 0 };
	hex_str[0] = hex[0];
	hex_str[1] = hex[1];
	return (hex_str[0] % 32 + 9) % 25 * 16 + (hex_str[1] % 32 + 9) % 25;
}

/** Hex string to bytes converter
 *  \param[in] hex a pointer to the hex string
 *  \param[in] hex_len the size of the string
 *  \param[inout] bytes the byte array to store the result in */
void ss_hex_string_to_bytes(const uint8_t *hex, size_t hex_len, uint8_t bytes[hex_len / 2])
{
	int i;

	for (i = 0; i < hex_len / 2; i++) {
		bytes[i] = ss_hex_to_uint8((char *)&hex[i * 2]);
	}
}

int softsim_get_key(char *secretkey, int size)
{
    ss_FILE fp = ss_fopen("ss_SIMKEY.key", "r");
    if (fp == NULL)
        return NULL;
    
    int readcount = ss_fread(secretkey, 1, size, fp);
	if (readcount != size)
		return NULL;

    ss_fclose(fp);
	// TODO: implement some backup file storage location. Provisioning should only be done once.
    return readcount;
}


/* Onomondo SoftSIM Profile Decoder
 * --------------------------------------------------------
 * This function is used to decode a SoftSIM profile, as exported by
 * the Onomondo SoftSIM CLI tool.
 *
 * For future compatibility we use TLV encoding for the profile
 * I.e. TAG | LEN | DATA[LEN] || TAG | LEN | DATA[LEN] || TAG | LEN | DATA[LEN] || ...
 *
 * Maximum string length of AT command tlv encoded hex string, when containing all tags
 * Byte count: Tag  + Len  + IMSI + ICCID + OPC + KIx  + SMSP + PINx + PUK
 * Byte count: 11x2 + 11x2 + 18   + 20    + 32  + 32x3 + 52   + 16x3 + 16 = 326 bytes */

/** Parse an TLV encoded string and get back the decoded struct.
 *  This decoder is made specifically to fit the Onomondo SoftSIM
 *  CLI tools decrypted output format.
 *  
 *  \param[in] input_string a pointer to the input data source of the profile.
 *  \param[in] len the length of the profile string.
 *  \param[in] profile a pointer to the receiving profile struct.
 *  \returns return 0 if valid profile is decoded. error code otherwise.
 */
uint8_t ss_profile_from_string(size_t len, const char input_string[], struct ss_profile *profile)
{
	*profile = (struct ss_profile){0};

	size_t pos = 0, data_end = 0, data_start = 0;
	uint8_t tag = 0, data_len = 0;

	while (pos < len - 2) {
		data_start = pos + 4;
		tag = ss_hex_to_uint8((char *)&input_string[pos]);
		data_len = ss_hex_to_uint8((char *)&input_string[pos + 2]);

		// advance to next tag
		data_end = data_start + data_len;
		pos = data_end;

		// bad encoding
		if (data_end > len) {
			return 1;
		}

		switch (tag) {
		case IMSI_TAG:
			if (!(data_len == (IMSI_LEN * 2))) {
				return 10;
			}
			ss_hex_string_to_bytes(&input_string[data_start], data_len, profile->_3F00_7ff0_6f07);
			break;
		case ICCID_TAG:
			if (!(data_len == (ICCID_LEN * 2))) {
				return 11;
			}
			ss_hex_string_to_bytes(&input_string[data_start], data_len, profile->_3F00_2FE2);
			break;
		case OPC_TAG:
			if (!(data_len == (KEY_SIZE * 2))) {
				return 12;
			}
			ss_hex_string_to_bytes(&input_string[data_start], data_len, &profile->_3F00_A001[KEY_SIZE]);
			break;
		case KI_TAG:
			if (!(data_len == (KEY_SIZE * 2))) {
				return 13;
			}
			ss_hex_string_to_bytes(&input_string[data_start], data_len, &profile->_3F00_A001[0]);
			ss_hex_string_to_bytes(&input_string[data_start], data_len, profile->k);
			break;
		case KIC_TAG:
			if (!(data_len == (KEY_SIZE * 2))) {
				return 14;
			}
			ss_hex_string_to_bytes(&input_string[data_start], data_len, &profile->_3F00_A004[A004_HEADER_SIZE]);
			ss_hex_string_to_bytes(&input_string[data_start], data_len, profile->kic);
			break;
		case KID_TAG:
			if (!(data_len == (KEY_SIZE * 2))) {
				return 15;
			}
			ss_hex_string_to_bytes(&input_string[data_start], data_len, &profile->_3F00_A004[A004_HEADER_SIZE + KEY_SIZE]);
			ss_hex_string_to_bytes(&input_string[data_start], data_len, profile->kid);
			break;
		case SMSP_TAG:
			if (!(data_len == (SMSP_RECORD_SIZE * 2))) {
				return 16;
			}
			ss_hex_string_to_bytes(&input_string[data_start], data_len, profile->SMSP);
			break;
		case PIN_1_TAG:
			if (!(data_len <= (PIN_SIZE * 2))) {
				break;
			}
			ss_hex_string_to_bytes(&input_string[data_start], data_len, &profile->_3F00_A003[0 * A003_RECORD_SIZE + PIN_OFFSET]);
			break;
		case PIN_2_TAG:
			if (!(data_len <= (PIN_SIZE * 2))) {
				break;
			}
			ss_hex_string_to_bytes(&input_string[data_start], data_len, &profile->_3F00_A003[1 * A003_RECORD_SIZE + PIN_OFFSET]);
			break;
		case PIN_ADM_TAG:
			if (!(data_len <= (PIN_SIZE * 2))) {
				break;
			}
			ss_hex_string_to_bytes(&input_string[data_start], data_len, &profile->_3F00_A003[2 * A003_RECORD_SIZE + PIN_OFFSET]);
			break;
		case PUK_TAG:
			if (!(data_len <= (PIN_SIZE * 2))) {
				break;
			}
			ss_hex_string_to_bytes(&input_string[data_start], data_len, &profile->_3F00_A003[0 * A003_RECORD_SIZE + PUK_OFFSET]);
			ss_hex_string_to_bytes(&input_string[data_start], data_len, &profile->_3F00_A003[1 * A003_RECORD_SIZE + PUK_OFFSET]);
			break;
		case END_TAG:
			// end of profile
			pos = len;
			break;
		default:
			// unknown tag, skip
			pos += data_len;
			break;
		}
	}

	profile->_3F00_A001[KEY_SIZE + KEY_SIZE] = '0';
	profile->_3F00_A001[KEY_SIZE + KEY_SIZE + 1] = '0';

	return 0; // valid profile decoded.
}


uint8_t write_profile_to_littlefs(const struct ss_profile *profile)
{
    size_t rc = 0;
    ss_FILE fp = NULL;

    // write ICCID
    fp = ss_fopen(ICCID_FILE, "w");
	if (fp != NULL)
	{
		rc = ss_fwrite(profile->_3F00_2FE2, 1, ICCID_LEN, fp);
		ss_fclose(fp);
	}

    // write IMSI
    fp = ss_fopen(IMSI_FILE, "w");
	if (fp != NULL)
	{
		rc = ss_fwrite(profile->_3F00_7ff0_6f07, 1, IMSI_LEN, fp);
		ss_fclose(fp);
	}
    
    // write A001
    fp = ss_fopen(A001_FILE, "w");
	if (fp != NULL)
	{
		rc = ss_fwrite(profile->_3F00_A001, 1, A001_LEN, fp);
		ss_fclose(fp);
	}

    // write A004
    fp = ss_fopen(A004_FILE, "w");
	if (fp != NULL)
	{
		rc = ss_fwrite(profile->_3F00_A004, 1, A004_LEN, fp);
		ss_fclose(fp);
	}

    // write SMSP
    fp = ss_fopen(SMSP_FILE, "w");
	if (fp != NULL)
	{
		rc = ss_fwrite(profile->SMSP, 1, SMSP_RECORD_SIZE, fp);
		ss_fclose(fp);
	}

    return rc;

exit:
    return rc;
}


uint8_t onomondo_profile_provisioning()
{
	int size_of_key_file = ss_file_size("ss_SIMKEY.key");
	
	if (size_of_key_file <= 100)
		return 10;
	
	char *new_profile = SS_ALLOC_N(size_of_key_file);
    struct ss_profile *decoded_profile = SS_ALLOC(*decoded_profile);

	if (softsim_get_key(new_profile, size_of_key_file) == NULL) {
		SS_FREE(new_profile);
		SS_FREE(decoded_profile);
		return 11;
	}

    uint8_t rc = ss_profile_from_string(size_of_key_file, new_profile, decoded_profile);

	SS_FREE(new_profile);

    if (rc != 0) {
		SS_FREE(decoded_profile);
        return 12;
	}

	rc = write_profile_to_littlefs(decoded_profile);
	if (rc == 0) {
		SS_FREE(decoded_profile);
		return 13;
	}

	SS_FREE(decoded_profile);
    return 0;
}
