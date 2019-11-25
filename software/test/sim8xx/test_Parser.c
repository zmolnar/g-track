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

static void printParser(SIM_Parser_t *parser)
{
    printf("atstart:     %d\n", parser->atstart);
    printf("statusstart: %d\n", parser->statusstart);
    printf("statusend:   %d\n", parser->statusend);
    printf("atend:       %d\n", parser->atend);
    printf("urcstart:    %d\n", parser->urcstart);
    printf("urcend:      %d\n\n", parser->urcend);
}


void test_ParserATonly(void)
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

void test_ParserATonlyIsAtMessage(void)
{
    char input[] = "AT+CGNSPWR=1\r\r\nOK\r\n";

    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT(SIM_ParserIsAtMessage(&parser));
    TEST_ASSERT_FALSE(SIM_ParserIsUrc(&parser));
}

void test_ParserGetAtMessage(void)
{
    char input[] = "AT+CGNSPWR=1\r\r\nOK\r\n";

    SIM_ParserProcessInput(&parser, input);

    char at[128] = {0};
    SIM_ParserGetAtMessage(&parser, at, sizeof(at));

    TEST_ASSERT_EQUAL_STRING(input, at);
}

void test_ParserURConly(void)
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

void test_ParserURConlyIsURC(void)
{
    char input[] = "\r\n+CPIN: NOT INSERTED\r\n";

    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT(SIM_ParserIsUrc(&parser));
    TEST_ASSERT_FALSE(SIM_ParserIsAtMessage(&parser));
}

void test_ParserGetURC(void)
{
    char input[] = "\r\n+CPIN: NOT INSERTED\r\n";

    SIM_ParserProcessInput(&parser, input);

    char urc[128] = {0};
    SIM_ParserGetUrc(&parser, urc, sizeof(urc));

    TEST_ASSERT_EQUAL_STRING(input, urc);
}

void test_ParserFirstATSecondURC(void)
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

void test_ParserFirstATSecondURC_IsAT_IsURC(void)
{
    char input[] = "AT+CGNSPWR=1\r\r\nOK\r\n\r\n+CPIN: NOT INSERTED\r\n";
    
    SIM_ParserProcessInput(&parser, input);

    TEST_ASSERT(SIM_ParserIsUrc(&parser));
    TEST_ASSERT(SIM_ParserIsAtMessage(&parser));
}

void test_ParserFirstATSecondURC_GetAT_GetURC(void)
{
    char input[] = "AT+CGNSPWR=1\r\r\nOK\r\n\r\n+CPIN: NOT INSERTED\r\n";
    
    SIM_ParserProcessInput(&parser, input);

    char at[128] = {0};
    SIM_ParserGetAtMessage(&parser, at, sizeof(at));

    TEST_ASSERT_EQUAL_STRING("AT+CGNSPWR=1\r\r\nOK\r\n", at);

    char urc[128] = {0};
    SIM_ParserGetUrc(&parser, urc, sizeof(urc));

    TEST_ASSERT_EQUAL_STRING("\r\n+CPIN: NOT INSERTED\r\n", urc);
}

void test_ParserFirstURCSecondAT(void)
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

void test_ParserFirstURCSecondAT_IsAT_IsURC(void)
{
    char input[] = "\r\n+CPIN: NOT INSERTED\r\nAT+CGNSPWR=1\r\r\nOK\r\n";
    
    SIM_ParserProcessInput(&parser, input);

    TEST_ASSERT(SIM_ParserIsUrc(&parser));
    TEST_ASSERT(SIM_ParserIsAtMessage(&parser));
}

void test_ParserFirstURCSecondAT_GetAT_GetURC(void)
{
    char input[] = "\r\n+CPIN: NOT INSERTED\r\nAT+CGNSPWR=1\r\r\nOK\r\n";
    
    SIM_ParserProcessInput(&parser, input);

    char at[128] = {0};
    SIM_ParserGetAtMessage(&parser, at, sizeof(at));

    TEST_ASSERT_EQUAL_STRING("AT+CGNSPWR=1\r\r\nOK\r\n", at);

    char urc[128] = {0};
    SIM_ParserGetUrc(&parser, urc, sizeof(urc));

    TEST_ASSERT_EQUAL_STRING("\r\n+CPIN: NOT INSERTED\r\n", urc);
}

void test_ParserATWithEmbeddedInvalidStatus(void)
{
    char input[] = "AT+CGNSPWR=1\r\nNOTASTATUS\r\n\r\r\nOK\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(0, parser.atstart);
    TEST_ASSERT_EQUAL(29, parser.statusstart);
    TEST_ASSERT_EQUAL(31, parser.statusend);
    TEST_ASSERT_EQUAL(33, parser.atend);
}

void test_ParserATWithEmbeddedInvalidStatus_isAT(void)
{
    char input[] = "AT+CGNSPWR=1\r\nNOTASTATUS\r\n\r\r\nOK\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT(SIM_ParserIsAtMessage(&parser));
}

void test_ParserATWithEmbeddedInvalidStatus_getStatus(void)
{
    char input[] = "AT+CGNSPWR=1\r\nNOTASTATUS\r\n\r\r\nOK\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_OK, SIM_ParserGetStatus(&parser));
}

void test_ParserATWithEmbeddedInvalidStatus_isATGetAtMessage(void)
{
    char input[] = "AT+CGNSPWR=1\r\nNOTASTATUS\r\n\r\r\nOK\r\n";

    SIM_ParserProcessInput(&parser, input);

    char at[128] = {0};
    SIM_ParserGetAtMessage(&parser, at, sizeof(at));

    TEST_ASSERT_EQUAL_STRING(input, at);
}

void test_ParserTestStatusOK(void)
{
    char input[] = "AT+CGNSPWR=1\r\r\nOK\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_OK, SIM_ParserGetStatus(&parser));
}

void test_ParserTestStatusCONNECT(void)
{
    char input[] = "AT+CGNSPWR=1\r\r\nCONNECT\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_CONNECT, SIM_ParserGetStatus(&parser));
}

void test_ParserTestStatusSEND_OK(void)
{
    char input[] = "AT+CGNSPWR=1\r\r\nSEND OK\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_SEND_OK, SIM_ParserGetStatus(&parser));
}

void test_ParserTestStatusSEND_FAIL(void)
{
    char input[] = "AT+CGNSPWR=1\r\r\nSEND FAIL\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_SEND_FAIL, SIM_ParserGetStatus(&parser));
}

void test_ParserTestStatusRING(void)
{
    char input[] = "AT+CGNSPWR=1\r\r\nRING\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_RING, SIM_ParserGetStatus(&parser));
}

void test_ParserTestStatusNO_CARRIER(void)
{
    char input[] = "AT+CGNSPWR=1\r\r\nNO CARRIER\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_NO_CARRIER, SIM_ParserGetStatus(&parser));
}

void test_ParserTestStatusERROR(void)
{
    char input[] = "AT+CGNSPWR=1\r\r\nERROR\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_ERROR, SIM_ParserGetStatus(&parser));
}

void test_ParserTestStatusNO_DIALTONE(void)
{
    char input[] = "AT+CGNSPWR=1\r\r\nNO DIALTONE\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_NO_DIALTONE, SIM_ParserGetStatus(&parser));
}

void test_ParserTestStatusBUSY(void)
{
    char input[] = "AT+CGNSPWR=1\r\r\nBUSY\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_BUSY, SIM_ParserGetStatus(&parser));
}

void test_ParserTestStatusNO_ANSWER(void)
{
    char input[] = "AT+CGNSPWR=1\r\r\nNO ANSWER\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_NO_ANSWER, SIM_ParserGetStatus(&parser));
}

void test_ParserTestStatusPROCEEDING(void)
{
    char input[] = "AT+CGNSPWR=1\r\r\nPROCEEDING\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_PROCEEDING, SIM_ParserGetStatus(&parser));
}

void test_ParserTestStatusWAIT_USER_DATA(void)
{
    char input[] = "AT+BTSPPSEND\r\r\n> ";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_WAITING_FOR_INPUT, SIM_ParserGetStatus(&parser));
}

void test_ParserTestStatusWAIT_USER_DATA_isAt(void)
{
    char input[] = "AT+BTSPPSEND\r\r\n> ";
    
    SIM_ParserProcessInput(&parser, input);
    
    printParser(&parser);

    TEST_ASSERT(SIM_ParserIsAtMessage(&parser));
    TEST_ASSERT_FALSE(SIM_ParserIsUrc(&parser));
}

void test_ParserTestStatusWAIT_USER_DATA_getAt(void)
{
    char input[] = "AT+BTSPPSEND\r\r\n> ";

    SIM_ParserProcessInput(&parser, input);

    char at[128] = {0};
    SIM_ParserGetAtMessage(&parser, at, sizeof(at));

    TEST_ASSERT_EQUAL_STRING(input, at);
}

void test_ParserTestStatus_UserData_SEND_OK(void)
{
    char input[] = "some user data with embedded \"> \" should not cause problems\r\nSEND OK\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_SEND_OK, SIM_ParserGetStatus(&parser));
}

void test_ParserTestStatus_UserData_isAT(void)
{
    char input[] = "some user data with embedded \"> \" should not cause problems\r\nSEND OK\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT(SIM_ParserIsAtMessage(&parser));
    TEST_ASSERT_FALSE(SIM_ParserIsUrc(&parser));
}

void test_ParserTestStatus_UserData_getAt(void)
{
    char input[] = "some user data with embedded \"> \" should not cause problems\r\nSEND OK\r\n";
    
    SIM_ParserProcessInput(&parser, input);

    char at[128] = {0};
    SIM_ParserGetAtMessage(&parser, at, sizeof(at));

    TEST_ASSERT_EQUAL_STRING(input, at);
}

void test_ParserTestStatus_UserDataSent_SEND_FAIL(void)
{
    char input[] = "some user data with embedded \"> \" should not cause problems\r\nSEND FAIL\r\n";
    
    SIM_ParserProcessInput(&parser, input);
    
    TEST_ASSERT_EQUAL(SIM8XX_SEND_FAIL, SIM_ParserGetStatus(&parser));
}
