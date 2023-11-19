#include "handler.h"
#include "../mcu_config.h"
#include "../utils/debug.h"
#include "../utils/output.h"
#include "../utils/beeper.h"
#include "../utils/led.h"
#include "tlv/parser.h"
#include "mqtt.h"

#include <Wire.h>
#include <Adafruit_PN532.h>
#include <esp_heap_trace.h>

// #define NUM_RECORDS 100
// static heap_trace_record_t trace_record[NUM_RECORDS];


#define RFID_READ_NO_TAG 0x00
#define RFID_READ_TIMED_OUT 0x01
#define RFID_READ_UID 0x10
#define RFID_READ_EMVCO 0x11

#define EMVCO_READ_OK 0x00
#define EMVCO_READ_FAIL 0x10

/**
 * Types of possible EMV combinations with other tag types
 * Per GSM Association Official Document SGP.12
 * "NFC Multi Protocol for Interoperability"
 *
 * https://www.gsma.com/newsroom/wp-content/uploads/SGP.12-v2.0.pdf
 */
#define EMVCO_UNSUPPORTED 0x00
#define EMVCO_BASIC 0x01
#define EMVCO_MF_CLASSIC 0x02
#define EMVCO_MF_DESFIRE 0x03
#define EMVCO_MF_CLASSIC_DESFIRE 0x04


Adafruit_PN532 nfc(PN532_IRQ, PN532_RST);

auto const NFCMutex = xSemaphoreCreateRecursiveMutex();
bool needToReactivate = false;

bool InitNFC()
{    

   // while (!xSemaphoreTakeRecursive(NFCMutex, 20000));

    Wire.setPins(PN532_SDA, PN532_SCL);
    Wire.setClock(10000);
    nfc.begin();


    // if (!nfc.getFirmwareVersion())
    // {
    //     StartBeep();
    //     DEBUG_PRINT("PN532 error\n");
    //     return false;
    // }
    uint32_t versiondata = nfc.getFirmwareVersion();
    if (! versiondata) {
        DEBUG_PRINT("Didn't find PN53x board");
        return false;
        // while (1); // halt
    }
    // Got ok data, print it out!
    DEBUG_PRINT("Found chip PN5 %x\n",(versiondata>>24) & 0xFF);

    //nfc.setRFCfg(0x7F);
    //nfc.setPassiveActivationRetries(0x01);
    //nfc.reset();
    nfc.SAMConfig();
   // xSemaphoreGiveRecursive(NFCMutex);

    return true;
}

uint8_t GetEMVCoType(uint8_t sak, uint16_t atqa)
{
    DEBUG_PRINT("SAK: %02x, ATQA: %04x\n", sak, atqa);
    // TODO
    //return EMVCO_BASIC;

    // Any EMVCo-capable targets must have SAK equal to 0x20 or 0x28
    if (sak != 0x20 && sak != 0x28)
    {
        return EMVCO_UNSUPPORTED;
    }

    // EMVCo Basic-only target
    if (atqa == 0x0004 && sak == 0x20)
    {
        return EMVCO_BASIC;
    }

    // EMVCo + Mifare Classic target
    if ((atqa == 0x0004 || atqa == 0x0044) && sak == 0x28)
    {
        return EMVCO_MF_CLASSIC;
    }

    // EMVCo + Mifare DESFire
    if (atqa == 0x0344 && sak == 0x20)
    {
        return EMVCO_MF_DESFIRE;
    }

    // EMVCo + Mifare Classic + Mifare DESFire
    if (atqa == 0x0344 && sak == 0x28)
    {
        return EMVCO_MF_CLASSIC_DESFIRE;
    }

    return EMVCO_UNSUPPORTED;
}

uint8_t ReadAndClassifyTarget(uint8_t *uid_buffer, uint8_t *uid_length)
{
    // byte buffer[64];
    // buffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
    // buffer[1] = 1; // Use only first detected target
    // buffer[2] = PN532_MIFARE_ISO14443A;

    // while (!xSemaphoreTakeRecursive(NFCMutex, 20000));

    // // Send passive targets inlist command
    // if (!nfc.sendCommandCheckAck(buffer, 3, 0))
    // {
    //     
    // }

    // if (!nfc.inListPassiveTarget()) {
    //     return RFID_READ_TIMED_OUT;
    // }

    bool success = nfc.readDetectedPassiveTargetID(uid_buffer, uid_length);
    // success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid_buffer, uid_length);

    //Send passive targets inlist command
    // вот это почему-то не работает с Apple Pay. Но при этом, если сделать inlist перед readdetectedpassivetargetid,
    // библиотека вешается, с I2C error -263 (timeout)
    if (!nfc.inListPassiveTarget()) {
        return RFID_READ_TIMED_OUT;
    }
    
    
    // bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid_buffer, uid_length, 100);
    
    // Waiting for the target detection response
    
    

    // if (!nfc.waitready(0))
    // {
    //     return RFID_READ_TIMED_OUT;
    // }

    // // Reading response to buffer
    // nfc.readdata(buffer, 20);

    // Check is any tags found
    if (!success)
    {
        return RFID_READ_NO_TAG;
    }
    StartLEDRing();


    // Parse ATQA, SAK & Inlisted Tag Tg
    uint8_t sak = nfc.sak;
    uint16_t atqa = nfc.atqa;

    uint8_t emvco_support = GetEMVCoType(nfc.sak, nfc.atqa);
    if (emvco_support == EMVCO_UNSUPPORTED)
    {
        return RFID_READ_UID;
    }
    else
    {
        return RFID_READ_EMVCO;
    }
   // xSemaphoreGive(NFCMutex);
}

void HexDump(const char *preamble, const uint8_t *buffer, uint16_t size)
{
    DEBUG_PRINT("%s: ", preamble);
    DEBUG_PRINT("size:%u ", size);
    for (uint16_t i = 0; i < size; i++)
    {
        DEBUG_PRINT("%02x", buffer[i]);
    }
    DEBUG_PRINT("\n");
}

void OutputHexData(const char* type, const uint8_t *buffer, uint16_t size)
{
    if (size > 2048) {
        return;
    }
    char result_buffer[4097] = {};
    for (uint16_t i = 0; i < size; i++) {
        snprintf(result_buffer + i * 2, 3, "%02X", buffer[i]);
    }
    OutputReadID(type, result_buffer);
    PublishToMQTT(type, result_buffer);
}

void OutputPan(const char* type, const uint8_t *buffer, uint16_t size)
{
    if (size > 512) {
        return;
    }
    char result_buffer[513] = {};
    for (uint16_t i = 0; i < size; i++) {
        snprintf(result_buffer + i, 2, "%c", buffer[i]);
    }
    OutputReadID(type, result_buffer);
    PublishToMQTT(type, result_buffer);
}

bool EMVGetAID(uint8_t *aid, uint8_t *aid_length)
{
    uint8_t ber_buffer[255];
    uint8_t ber_length = 255;
    //while (!xSemaphoreTakeRecursive(NFCMutex, 20000));

    // Read PPSE
    // Header: 0x00 0xA4 0x04 0x00, Data: 2PAY.SYS.DDF01
    uint8_t apdu[] = {0x00, 0xA4, 0x04, 0x00, 0x0e, 0x32, 0x50, 0x41, 0x59, 0x2e, 0x53, 0x59, 0x53, 0x2e, 0x44, 0x44, 0x46, 0x30, 0x31, 0x00};

    DEBUG_PRINT("EXCH... ");
    unsigned long tme = millis();
    if (!nfc.inDataExchange(apdu, sizeof(apdu), ber_buffer, &ber_length))
    {
        return false;
    }
    DEBUG_PRINT("! %d\n", millis() - tme);
    HexDump("AID_PACKET", ber_buffer, ber_length);

    Parser tlv_parser(ber_buffer, ber_length);
    if (tlv_parser.IsNull())
    {
        DEBUG_PRINT("Failed tlv\n");
        return false;
    }

    Parser fci_parser = tlv_parser.GetObject(0x6F);
    if (fci_parser.IsNull())
    {
        DEBUG_PRINT("Failed 0x6F\n");
        return false;
    }

    Parser fci_prop_template = fci_parser.GetObject(0xA5);
    if (fci_prop_template.IsNull())
    {
        DEBUG_PRINT("Failed 0xA5\n");
        return false;
    }

    Parser fci_issuer_table = fci_prop_template.GetObject(0xBF0C);
    if (fci_issuer_table.IsNull())
    {
        DEBUG_PRINT("Failed 0xBF0C\n");
        return false;
    }

    Parser app_template = fci_issuer_table.GetObject(0x61);
    if (app_template.IsNull())
    {
        DEBUG_PRINT("Failed 0x61\n");
        return false;
    }

    Parser aid_data = app_template.GetObject(0x4F);
    if (aid_data.IsNull())
    {
        DEBUG_PRINT("Failed 0x4F\n");
        return false;
    }

    if (aid_data.GetLength() > 255)
    {
        DEBUG_PRINT("Failed Length > 255");
        return false;
    }

    memcpy(aid, aid_data.GetData(), aid_data.GetLength());
    *aid_length = aid_data.GetLength();

    //xSemaphoreGive(NFCMutex);
    return true;
}

bool EMVGetPDOLAnswer(uint8_t *aid, uint16_t aid_length, uint8_t *pdol_answer, uint8_t *pdol_answer_length)
{
    uint8_t ber_buffer[255] = {};
    memset(ber_buffer, 255, 0xAA);
    uint8_t ber_length = 255;

    if (5 + aid_length + 1 > 255)
    {
        return false;
        DEBUG_PRINT("meow\n aid length wrong\n");
    }

    // Select AID
    // Header: 0x00 0xA4 0x04 0x00, Data: AID
    uint8_t apdu[255] = {
        0x00, 0xA4, 0x04, 0x00,
        (uint8_t)(aid_length)};

    memcpy(apdu + 5, aid, aid_length);
    apdu[5 + aid_length] = 0;

    DEBUG_PRINT("EXCH... ");
    unsigned long tme = millis();
    //while (!xSemaphoreTakeRecursive(NFCMutex, 20000));
    if (!nfc.inDataExchange(apdu, 5 + aid_length + 1, ber_buffer, &ber_length))
    {
        return false;
    }
    DEBUG_PRINT("! %d\n", millis() - tme);
    //xSemaphoreGiveRecursive(NFCMutex);

    HexDump("PDOL_PACKET", ber_buffer, ber_length);

    memcpy(pdol_answer, ber_buffer, ber_length);
    *pdol_answer_length = ber_length;
    return true;
}

bool EMVGetPDOL(uint8_t *pdol_answer, uint16_t pdol_answer_length, uint8_t *pdol, uint8_t *pdol_length)
{
    Parser tlv_parser(pdol_answer, pdol_answer_length);
    if (tlv_parser.IsNull())
    {
        DEBUG_PRINT("Failed tlv\n");
        return false;
    }

    Parser fci_parser = tlv_parser.GetObject(0x6F);
    if (fci_parser.IsNull())
    {
        DEBUG_PRINT("Failed 0x6F\n");
        return false;
    }

    Parser fci_prop_template = fci_parser.GetObject(0xA5);
    if (fci_prop_template.IsNull())
    {
        DEBUG_PRINT("Failed 0xA5\n");
        return false;
    }

    Parser pdol_data = fci_prop_template.GetObject(0x9F38);
    if (pdol_data.IsNull())
    {
        *pdol_length = 0;
        return true;
    }

    if (pdol_data.GetLength() > 255)
    {
        return false;
    }

    memcpy(pdol, pdol_data.GetData(), pdol_data.GetLength());
    *pdol_length = pdol_data.GetLength();
    return true;
}

bool EMVGetDataByPDOL(uint8_t *pdol, uint16_t pdol_length, uint8_t *data, uint8_t *data_length)
{
    uint8_t ber_buffer[255];
    uint8_t ber_length = 255;

    if (8 + pdol_length > 255)
    {
        return false;
    }

    // Select GPO
    // Header: 0x00 0xA4 0x04 0x00 len+2 0x83 len, Data: PDOL
    uint8_t apdu[255] = {
        0x80, 0xA8, 0x00, 0x00,
        (uint8_t)(pdol_length + 2), 0x83, (uint8_t)(pdol_length)};

    memcpy(apdu + 7, pdol, pdol_length);
    apdu[pdol_length + 7] = 0;

    HexDump("PDOL_OUT", apdu, 8 + pdol_length);

    DEBUG_PRINT("EXCH... ");
    unsigned long tme = millis();
   //while (!xSemaphoreTakeRecursive(NFCMutex, 20000));
    if (!nfc.inDataExchange(apdu, 8 + pdol_length, ber_buffer, &ber_length))
    {
        return false;
    }
    //xSemaphoreGiveRecursive(NFCMutex);

    DEBUG_PRINT("! %d\n", millis() - tme);

    memcpy(data, ber_buffer, ber_length);
    *data_length = ber_length;
    return true;
}

struct PDOLValue
{
    uint16_t tag;
    uint8_t length;
    uint8_t data[255];
};

// source: https://github.com/flipperdevices/flipperzero-firmware/blob/dev/lib/nfc/protocols/emv.c
PDOLValue fixed_values[] = {
    {0x9F59, 3, {0xC8, 0x80, 0x00}},
    {0x9F58, 1, {0x01}},
    /* {0x9F66, 4, {0xf9, 0x00, 0x40, 0x80}}, */
    {0x9F40, 4, {0x79, 0x00, 0x40, 0x80}},
    {0x9F02, 6, {0x00, 0x00, 0x00, 0x10, 0x00, 0x00}},
    {0x9F1A, 2, {0x01, 0x24}},
    {0x5F2A, 2, {0x01, 0x24}},
    {0x009A, 3, {0x19, 0x01, 0x01}},
    {0x9F37, 4, {0x82, 0x3D, 0xDE, 0x7A}},
    {0x9F66, 4, {0xf0, 0x00, 0x00, 0x00}}};

bool EMVGenerateFakePDOL(uint8_t *pdol_in, uint8_t pdol_in_length,
                         uint8_t *pdol_out, uint8_t *pdol_out_length)
{
    uint8_t in_index = 0;
    uint8_t out_index = 0;
    while (in_index < pdol_in_length)
    {
        uint16_t tag = pdol_in[in_index++];
        if ((tag & 31) == 31)
        {
            tag <<= 8;
            tag |= pdol_in[in_index++];
        }
        uint16_t length = pdol_in[in_index++];
        if ((length & 128) == 128)
        {
            length &= 0x7F;
            length <<= 8;
            length |= pdol_in[in_index++];
        }

        bool found = false;
        for (uint32_t i = 0; i < sizeof(fixed_values) / sizeof(PDOLValue); i++)
        {
            if (fixed_values[i].tag == tag && fixed_values[i].length == length)
            {
                found = true;
                DEBUG_PRINT("Found tag %04x with length %d\n", tag, length);
                if (out_index + length > 255)
                {
                    return false;
                }
                memcpy(pdol_out + out_index, fixed_values[i].data, length);
                out_index += length;
                break;
            }
        }

        if (!found)
        {
            DEBUG_PRINT("Not found tag %04x with length %d\n", tag, length);
            if (out_index + length > 255)
            {
                return false;
            }
            memset(pdol_out + out_index, 0, length);
            out_index += length;
        }
    }
    *pdol_out_length = out_index;
    return true;
}

struct PANPath
{
    uint8_t length;
    uint16_t tags[255];
};

PANPath pan_paths[] = {
    {2, {0x77, 0x57}},
    {4, {0x6F, 0xA5, 0xBF62, 0x57}},
    {2, {0x70, 0x57}},
    {2, {0x70, 0x5A}},
};

bool VerifyLuhn(uint8_t *raw_digits, uint8_t length)
{
    uint8_t parity = length % 2;
    uint8_t sum = 0;
    for (uint8_t i = 0; i < length; i++)
    {
        if (i % 2 != parity)
        {
            sum += raw_digits[i];
        }
        else if (raw_digits[i] > 4)
        {
            sum += raw_digits[i] * 2 - 9;
        }
        else
        {
            sum += raw_digits[i] * 2;
        }
    }
    return sum % 10 == 0;
}

bool EMVGetPanFromData(uint8_t *data, uint8_t data_length, uint8_t *pan, uint8_t *pan_length)
{
    HexDump("DATAA", data, data_length);
    Parser tlv_parser(data, data_length);
    if (tlv_parser.IsNull())
    {
        DEBUG_PRINT("Failed tlv\n");
        return false;
    }

    heap_caps_check_integrity_all(true);

    for (uint8_t i = 0; i < sizeof(pan_paths) / sizeof(PANPath); i++)
    {
        heap_caps_check_integrity_all(true);
        Parser current_parser = tlv_parser;
        for (uint8_t j = 0; j < pan_paths[i].length; j++)
        {
            heap_caps_check_integrity_all(true);
            current_parser = current_parser.GetObject(pan_paths[i].tags[j]);
            if (current_parser.IsNull())
            {
                break;
            }
        }
        heap_caps_check_integrity_all(true);
        if (current_parser.IsNull())
        {
            continue;
        }

        if (current_parser.GetLength() < 7)
        {
            continue;
        }

        uint8_t digits[255];
        uint8_t current_length = 0;

        HexDump("RAW_RECORD", current_parser.GetData(), current_parser.GetLength());

        for (uint8_t j = 0; j < current_parser.GetLength(); j++)
        {
            uint8_t digit_1 = current_parser.GetData()[j] >> 4;
            uint8_t digit_2 = current_parser.GetData()[j] & 0x0F;

            if (digit_1 >= 10)
            {
                break;
            }

            digits[current_length++] = digit_1;

            if (digit_2 >= 10)
            {
                break;
            }

            digits[current_length++] = digit_2;

            if (current_length > 20)
            {
                current_length = 0;
                break;
            }
            heap_caps_check_integrity_all(true);
        }

        if (current_length < 13 || current_length > 19)
        {
            continue;
        }

        HexDump("LUHN_CHECK", digits, current_length);

        if (VerifyLuhn(digits, current_length))
        {
            for (uint8_t j = 0; j < current_length; j++)
            {
                pan[j] = digits[j] + '0';
            }
            *pan_length = current_length;
            heap_caps_check_integrity_all(true);
            return true;
        } else {
            DEBUG_PRINT("Luhn failed\n");
        }
    }

    return false;
}

bool EMVGetAFLFromData(uint8_t *data, uint8_t data_length, uint8_t *afl, uint8_t *afl_length)
{
    Parser tlv_parser(data, data_length);
    if (tlv_parser.IsNull())
    {
        return false;
    }
    Parser rmtf2 = tlv_parser.GetObject(0x77);
    if (rmtf2.IsNull())
    {
        return false;
    }
    Parser afl_data = rmtf2.GetObject(0x94);
    if (afl_data.IsNull())
    {
        return false;
    }
    memcpy(afl, afl_data.GetData(), afl_data.GetLength());
    *afl_length = afl_data.GetLength();
    return true;
}

bool EMVReadRecord(uint8_t sfi, uint8_t record_id, uint8_t* record_data, uint8_t* record_length) 
{
    // Read record
    // Header: 0x00 0xA4 0x04 0x00 len+2 0x83 len, Data: PDOL
    uint8_t sfi_param = (sfi << 3) | (1 << 2);

    uint8_t apdu[5] = {0x00, 0xB2, record_id, sfi_param, 0x00};

   // while (!xSemaphoreTakeRecursive(NFCMutex, 20000));

    HexDump("APDU", apdu, sizeof(apdu));
    DEBUG_PRINT("EXCH... ");
    unsigned long tme = millis();

    if (!nfc.inDataExchange(apdu, sizeof(apdu), record_data, record_length))
    {
        return false;
    }
    
    DEBUG_PRINT("! %d\n", millis() - tme);
    //xSemaphoreGiveRecursive(NFCMutex);

    return true;
}

bool EMVGetPanFromAFL(uint8_t *afl, uint8_t afl_length, uint8_t *pan, uint8_t *pan_length)
{
    for (uint8_t i = 0; i < afl_length; i += 4) 
    {
        uint8_t sfi = afl[i] >> 3;
        uint8_t record_start = afl[i + 1];
        uint8_t record_end = afl[i + 2];
        heap_caps_check_integrity_all(true);
        for (uint8_t j = record_start; j <= record_end; j++) 
        {
            uint8_t record[255];
            uint8_t record_length = 255;
            heap_caps_check_integrity_all(true);
            if (!EMVReadRecord(sfi, j, record, &record_length)) {
                continue;
            }

            heap_caps_dump_all;

            DEBUG_PRINT("Record %d, %d: ", sfi, j);
            HexDump("RECORD", record, record_length);

            if (EMVGetPanFromData(record, record_length, pan, pan_length)) {
                heap_caps_check_integrity_all(true);
                return true;
            }
        }
    }
    return false;
}

uint8_t ReadEMVCoPAN(uint8_t* pan, uint8_t* pan_length)
{
    heap_caps_check_integrity_all(true);
    uint8_t aid[255] = {};
    uint8_t aid_length;
    if (!EMVGetAID(aid, &aid_length))
    {
        DEBUG_PRINT("Failed to get AID\n");
        return EMVCO_READ_FAIL;
    }
    heap_caps_check_integrity_all(true);

    HexDump("AID", aid, aid_length);

    uint8_t pdol_answer[255] = {};
    uint8_t pdol_answer_length;
    if (!EMVGetPDOLAnswer(aid, aid_length, pdol_answer, &pdol_answer_length))
    {
        DEBUG_PRINT("Failed to get PDOL Answer\n");
        return EMVCO_READ_FAIL;
    }
    heap_caps_check_integrity_all(true);

    if (EMVGetPanFromData(pdol_answer, pdol_answer_length, pan, pan_length))
    {
        HexDump("PAN", pan, *pan_length);
        return EMVCO_READ_OK;
    }
    heap_caps_check_integrity_all(true);

    uint8_t pdol[255] = {};
    uint8_t pdol_length;
    if (!EMVGetPDOL(pdol_answer, pdol_answer_length, pdol, &pdol_length))
    {
        DEBUG_PRINT("Failed to get PDOL\n");
        return EMVCO_READ_FAIL;
    }
    HexDump("PDOL", pdol, pdol_length);

    heap_caps_check_integrity_all(true);

    uint8_t pdol_out[255] = {};
    uint8_t pdol_out_length;
    if (!EMVGenerateFakePDOL(pdol, pdol_length, pdol_out, &pdol_out_length))
    {
        DEBUG_PRINT("Failed to generate fake PDOL\n");
        return EMVCO_READ_FAIL;
    }

    heap_caps_check_integrity_all(true);

    uint8_t data[256] = {};
    uint8_t data_length;

    if (!EMVGetDataByPDOL(pdol_out, pdol_out_length, data, &data_length))
    {
        DEBUG_PRINT("Failed to get data by PDOL\n");
        return EMVCO_READ_FAIL;
    }

    HexDump("DATA", data, data_length);

    heap_caps_check_integrity_all(true);

    if (EMVGetPanFromData(data, data_length, pan, pan_length))
    {
        HexDump("PAN", pan, *pan_length);
        return EMVCO_READ_OK;
    }

    heap_caps_check_integrity_all(true);

    uint8_t afl[512] = {};
    uint8_t afl_length;
    
    if (!EMVGetAFLFromData(data, data_length, afl, &afl_length))
    {
        DEBUG_PRINT("Failed to get AFL from data\n");
        return EMVCO_READ_FAIL;
    }

    HexDump("AFL", afl, afl_length);

    heap_caps_check_integrity_all(true);

    if (!EMVGetPanFromAFL(afl, afl_length, pan, pan_length))
    {
        DEBUG_PRINT("Failed to get PAN from AFL\n");
        return EMVCO_READ_FAIL;
    }

    HexDump("PAN", pan, *pan_length);

    heap_caps_check_integrity_all(true);


    return EMVCO_READ_OK;
}

const uint32_t success_beeps[] = {3000, 50};
const uint32_t emv_beeps[] = {4000, 50, 0, 75, 4000, 50};
const uint32_t fail_beeps[] = {1000, 200, 0, 200, 1000, 200};

void HandleNFC()
{
    //InitNFC();
    DEBUG_PRINT("NFC started on core %d\n", xPortGetCoreID());

    uint8_t uid_length;
    uint8_t uid[7];

    bool readerEnabled = false;
    bool cardDetected = false;
    unsigned long lastRead;
    for (;;)
    {   
        if(!readerEnabled && millis() - lastRead > 2000) {
            DEBUG_PRINT("Starting card detection...");
            readerEnabled = true;
            //nfc.wakeup();
            if(nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A)) cardDetected = true;
            DEBUG_PRINT(" ok.\n");
        }

        if(!digitalRead(PN532_IRQ) && readerEnabled) cardDetected = true;

        //if(true) {
        if(cardDetected) {
            DEBUG_PRINT("<<< Found card >>>\n");
            lastRead = millis();
            cardDetected = false;
            readerEnabled = false;
            // if(!nfc.readDetectedPassiveTargetID(uid, &uid_length)) {
            //     DEBUG_PRINT("Meh. Nothing.\n");
            //     continue;
            // }

            uint8_t read_status = ReadAndClassifyTarget(uid, &uid_length);
            // bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uid_length);
            
            // uint8_t read_status = 0;
            
            DEBUG_PRINT("READ done %d %d %d\n", uid_length, nfc.sak, nfc.atqa);
            // nfc.PrintHex(uid, uid_length);

            switch (read_status)
            {
            case RFID_READ_NO_TAG:
                DEBUG_PRINT("No tag\n");
                StopLEDRing();
                break;
            case RFID_READ_TIMED_OUT:
                DEBUG_PRINT("Timed out\n");
                ErrorLED();
                Beep(fail_beeps, 5);
                break;
            case RFID_READ_UID:
                HexDump("UID", uid, uid_length);
                OutputHexData("UID", uid, uid_length);
                Beep(success_beeps, sizeof(success_beeps) / sizeof(success_beeps[0]));
                StopLEDRing();
                SuccessLED(1);
                break;
            case RFID_READ_EMVCO:
                //StartBeep();
                uint8_t pan[255];
                uint8_t pan_length = 20;
                if (ReadEMVCoPAN(pan, &pan_length) == EMVCO_READ_OK) {
                    DEBUG_PRINT("Got EMV PAN: %s\n", pan);
                    OutputPan("PAN", pan, pan_length);
                    //StopBeep();
                    SuccessLED(0);
                    Beep(emv_beeps, 6);
                } else {
                    DEBUG_PRINT("Failed to EMV\n");
                    OutputHexData("UID", uid, uid_length);
                    //StopBeep();
                    ErrorLED();
                    Beep(fail_beeps, 6);
                }
                break;

            // Throttle card reads just a little bit, wait for target
            // being removed from field or cooldown to pass
            while (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uid_length, 2000));
            }
            lastRead = millis();
        }
    }
}