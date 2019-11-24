#include "unity.h"

#include "Parser.h"
#include <string.h>

static SIM_Parser_t parser;

void setUp(void)
{
    memset(&parser, 0, sizeof(parser));
}

void tearDown(void)
{
}

void test_aturcparser_ATonly(void)
{
    char input[] = "AT+CGNSPWR=1\r\r\nOK\r\n";
    
    SIM_ParserProcessInput(&parser, input);

    TEST_ASSERT_EQUAL(0, parser.atstart);
    TEST_ASSERT_EQUAL(15, parser.statusstart);
    TEST_ASSERT_EQUAL(17, parser.statusend);
    TEST_ASSERT_EQUAL(19, parser.atend);
    TEST_ASSERT_EQUAL(0, parser.urcstart);
    TEST_ASSERT_EQUAL(0, parser.urcend);
}

void test_aturcparser_ATonlyIsAtMessage(void)
{
    char input[] = "AT+CGNSPWR=1\r\r\nOK\r\n";

    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT(SIM_ParserIsAtMessage(&parser));
    TEST_ASSERT_FALSE(SIM_ParserIsUrc(&parser));
}

void test_aturcparser_URConly(void)
{
    char input[] = "\r\n+CPIN: NOT INSERTED\r\n";
    
    SIM_ParserProcessInput(&parser, input);

    TEST_ASSERT_EQUAL(0, parser.urcstart);
    TEST_ASSERT_EQUAL(23, parser.urcend);
    TEST_ASSERT_EQUAL(0, parser.atstart);
    TEST_ASSERT_EQUAL(0, parser.statusstart);
    TEST_ASSERT_EQUAL(0, parser.statusend);
    TEST_ASSERT_EQUAL(0, parser.atend);
}

void test_aturcparser_URConlyIsURC(void)
{
    char input[] = "\r\n+CPIN: NOT INSERTED\r\n";

    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT(SIM_ParserIsUrc(&parser));
    TEST_ASSERT_FALSE(SIM_ParserIsAtMessage(&parser));
}

void test_aturcparser_FirstATSecondURC(void)
{
    char input[] = "AT+CGNSPWR=1\r\r\nOK\r\n\r\n+CPIN: NOT INSERTED\r\n";
    
    SIM_ParserProcessInput(&parser, input);

    TEST_ASSERT_EQUAL(0, parser.atstart);
    TEST_ASSERT_EQUAL(15, parser.statusstart);
    TEST_ASSERT_EQUAL(17, parser.statusend);
    TEST_ASSERT_EQUAL(19, parser.atend);
    TEST_ASSERT_EQUAL(19, parser.urcstart);
    TEST_ASSERT_EQUAL(42, parser.urcend);
}

void test_aturcparser_FirstATSecondURC_IsAT_IsURC(void)
{
    char input[] = "AT+CGNSPWR=1\r\r\nOK\r\n\r\n+CPIN: NOT INSERTED\r\n";
    
    SIM_ParserProcessInput(&parser, input);

    TEST_ASSERT(SIM_ParserIsUrc(&parser));
    TEST_ASSERT(SIM_ParserIsAtMessage(&parser));
}
#if 1
void test_aturcparser_FirstURCSecondAT(void)
{
    char input[] = "\r\n+CPIN: NOT INSERTED\r\nAT+CGNSPWR=1\r\r\nOK\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(0, parser.urcstart);
    TEST_ASSERT_EQUAL(23, parser.urcend);
    TEST_ASSERT_EQUAL(23, parser.atstart);
    TEST_ASSERT_EQUAL(38, parser.statusstart);
    TEST_ASSERT_EQUAL(40, parser.statusend);
    TEST_ASSERT_EQUAL(42, parser.atend);
}

void test_aturcparser_FirstURCSecondATWithEmbeddedNotStatus(void)
{
    char input[] = "\r\n+CPIN: NOT INSERTED\r\nAT+CGNSPWR=1\r\nNOTASTATUS\r\n\r\r\nOK\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(0, parser.urcstart);
    TEST_ASSERT_EQUAL(23, parser.urcend);
    TEST_ASSERT_EQUAL(23, parser.atstart);
    TEST_ASSERT_EQUAL(52, parser.statusstart);
    TEST_ASSERT_EQUAL(54, parser.statusend);
    TEST_ASSERT_EQUAL(56, parser.atend);
}

void test_aturcparser_TestStatusOK(void)
{
    char input[] = "\r\n+CPIN: NOT INSERTED\r\nAT+CGNSPWR=1\r\nNOTASTATUS\r\n\r\r\nOK\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_OK, SIM_ParserGetStatus(&parser));
}

void test_aturcparser_TestStatusCONNECT(void)
{
    char input[] = "\r\n+CPIN: NOT INSERTED\r\nAT+CGNSPWR=1\r\nNOTASTATUS\r\n\r\r\nCONNECT\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_CONNECT, SIM_ParserGetStatus(&parser));
}

void test_aturcparser_TestStatusSEND_OK(void)
{
    char input[] = "\r\n+CPIN: NOT INSERTED\r\nAT+CGNSPWR=1\r\nNOTASTATUS\r\n\r\r\nSEND OK\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_SEND_OK, SIM_ParserGetStatus(&parser));
}

void test_aturcparser_TestStatusSEND_FAIL(void)
{
    char input[] = "\r\n+CPIN: NOT INSERTED\r\nAT+CGNSPWR=1\r\nNOTASTATUS\r\n\r\r\nSEND FAIL\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_SEND_FAIL, SIM_ParserGetStatus(&parser));
}

void test_aturcparser_TestStatusRING(void)
{
    char input[] = "\r\n+CPIN: NOT INSERTED\r\nAT+CGNSPWR=1\r\nNOTASTATUS\r\n\r\r\nRING\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_RING, SIM_ParserGetStatus(&parser));
}

void test_aturcparser_TestStatusNO_CARRIER(void)
{
    char input[] = "\r\n+CPIN: NOT INSERTED\r\nAT+CGNSPWR=1\r\nNOTASTATUS\r\n\r\r\nNO CARRIER\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_NO_CARRIER, SIM_ParserGetStatus(&parser));
}

void test_aturcparser_TestStatusERROR(void)
{
    char input[] = "\r\n+CPIN: NOT INSERTED\r\nAT+CGNSPWR=1\r\nNOTASTATUS\r\n\r\r\nERROR\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_ERROR, SIM_ParserGetStatus(&parser));
}

void test_aturcparser_TestStatusNO_DIALTONE(void)
{
    char input[] = "\r\n+CPIN: NOT INSERTED\r\nAT+CGNSPWR=1\r\nNOTASTATUS\r\n\r\r\nNO DIALTONE\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_NO_DIALTONE, SIM_ParserGetStatus(&parser));
}

void test_aturcparser_TestStatusBUSY(void)
{
    char input[] = "\r\n+CPIN: NOT INSERTED\r\nAT+CGNSPWR=1\r\nNOTASTATUS\r\n\r\r\nBUSY\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_BUSY, SIM_ParserGetStatus(&parser));
}

void test_aturcparser_TestStatusNO_ANSWER(void)
{
    char input[] = "\r\n+CPIN: NOT INSERTED\r\nAT+CGNSPWR=1\r\nNOTASTATUS\r\n\r\r\nNO ANSWER\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_NO_ANSWER, SIM_ParserGetStatus(&parser));
}

void test_aturcparser_TestStatusPROCEEDING(void)
{
    char input[] = "\r\n+CPIN: NOT INSERTED\r\nAT+CGNSPWR=1\r\nNOTASTATUS\r\n\r\r\nPROCEEDING\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_PROCEEDING, SIM_ParserGetStatus(&parser));
}

void test_aturcparser_TestStatusWAIT_USER_DATA(void)
{
    char input[] = "\r\n+CPIN: NOT INSERTED\r\nAT+CGNSPWR=1\r\nNOTASTATUS\r\n\r> ";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_WAITING_FOR_INPUT, SIM_ParserGetStatus(&parser));
}

void test_aturcparser_TestStatusWAIT_USER_DATA2(void)
{
    char input[] = "AT+BTSPPSEND\r> ";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_WAITING_FOR_INPUT, SIM_ParserGetStatus(&parser));
}

#endif