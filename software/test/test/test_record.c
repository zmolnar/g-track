
#include "unity.h"
#include "Record.h"

void setUp(void)
{
}

void tearDown(void)
{
}


void test_REC_PushRecord(void)
{
	Record_t record;
	RecordBuffer_t recbuf;
	
	REC_EmptyBuffer(&recbuf);
	
	size_t i;
	for (i = 0; i < BUFFER_LENGTH-1; ++i) {
		REC_PushRecord(&recbuf, &record);
	
		TEST_ASSERT_EQUAL((i+1) % BUFFER_LENGTH, recbuf.wrindex);
		TEST_ASSERT_EQUAL(0, recbuf.rdindex);
	}
	
	for (i = BUFFER_LENGTH-1; i < 20; ++i) {
		REC_PushRecord(&recbuf, &record);
	
		TEST_ASSERT_EQUAL((i+1) % BUFFER_LENGTH, recbuf.wrindex);
		TEST_ASSERT_EQUAL((recbuf.wrindex+1) % BUFFER_LENGTH, recbuf.rdindex);
	}
}

void test_REC_GetSize(void)
{
	Record_t record;
	RecordBuffer_t recbuf;
	
	REC_EmptyBuffer(&recbuf);
	
	size_t i;
	
	// Not full
	for (i = 0; i < BUFFER_LENGTH - 1; ++i) {
		REC_PushRecord(&recbuf, &record);
		TEST_ASSERT_EQUAL(i+1, REC_GetSize(&recbuf));
	}
	
	
	// Full
	for (; i < 20; ++i) {
		REC_PushRecord(&recbuf, &record);
		TEST_ASSERT_EQUAL(BUFFER_LENGTH - 1, REC_GetSize(&recbuf));
	}	
}

void test_REC_GetNumOfUnsentRecords(void)
{
	Record_t record;
	RecordBuffer_t recbuf;
	
	REC_EmptyBuffer(&recbuf);
	
	size_t i;
	
	// 3 sent records
	for (i = 0; i < 3; ++i) {
		REC_PushRecord(&recbuf, &record);
		recbuf.records[i].isSent = true;
	}
	
	// 2 not sent records
	for (i = 3; i < 5; ++i) {
		REC_PushRecord(&recbuf, &record);
		recbuf.records[i].isSent = false;
	}

	size_t count = REC_GetNumOfUnsentRecords(&recbuf);
	TEST_ASSERT_EQUAL(2, count);
}

void test_REC_PopRecordIfSent(void)
{
	Record_t record;
	RecordBuffer_t recbuf;
	
	REC_EmptyBuffer(&recbuf);
	
	size_t i;
	for (i = 0; i < 3; ++i) {
		REC_PushRecord(&recbuf, &record);
		recbuf.records[i].isSent = true;
	}

	for (; i < BUFFER_LENGTH-1; ++i) {
		REC_PushRecord(&recbuf, &record);
		recbuf.records[i].isSent = false;
	}

  // 1. record is sent
	TEST_ASSERT_EQUAL(BUFFER_LENGTH-1, recbuf.wrindex);
	TEST_ASSERT_EQUAL(0, recbuf.rdindex);
	TEST_ASSERT(REC_PopRecordIfSent(&recbuf));

  // 2. record is sent
	TEST_ASSERT_EQUAL(BUFFER_LENGTH-1, recbuf.wrindex);
	TEST_ASSERT_EQUAL(1, recbuf.rdindex);
	TEST_ASSERT(REC_PopRecordIfSent(&recbuf));

  // 3. record is sent
	TEST_ASSERT_EQUAL(BUFFER_LENGTH-1, recbuf.wrindex);
	TEST_ASSERT_EQUAL(2, recbuf.rdindex);
	TEST_ASSERT(REC_PopRecordIfSent(&recbuf));

  // 4. record is not sent
	TEST_ASSERT_EQUAL(BUFFER_LENGTH-1, recbuf.wrindex);
	TEST_ASSERT_EQUAL(3, recbuf.rdindex);
	TEST_ASSERT_FALSE(REC_PopRecordIfSent(&recbuf));

  // 5. record is not sent
	TEST_ASSERT_EQUAL(BUFFER_LENGTH-1, recbuf.wrindex);
	TEST_ASSERT_EQUAL(3, recbuf.rdindex);
	TEST_ASSERT_FALSE(REC_PopRecordIfSent(&recbuf));
}

void test_REC_GetNextRecordAndMarkAsSent(void)
{
	Record_t record;
	RecordBuffer_t recbuf;
	
	REC_EmptyBuffer(&recbuf);
	
	size_t i;
	for (i = 0; i < 3; ++i)  {
		REC_PushRecord(&recbuf, &record);
		recbuf.records[i].isSent = true;
	}

	for (; i < BUFFER_LENGTH-1; ++i) {
		REC_PushRecord(&recbuf, &record);
		recbuf.records[i].isSent = false;
	}

	for (i = 3; i < BUFFER_LENGTH - 1; ++i)	{
		Record_t *prec = NULL;
		TEST_ASSERT(REC_GetNextRecordAndMarkAsSent(&recbuf, &prec));
		TEST_ASSERT_EQUAL_PTR(&recbuf.records[i], prec);
		TEST_ASSERT(recbuf.records[i].isSent);
		TEST_ASSERT_EQUAL(BUFFER_LENGTH -1, recbuf.wrindex);
		TEST_ASSERT_EQUAL(0, recbuf.rdindex);
	}
}

void test_REC_CancelLastTransaction(void)
{
	Record_t record;
	RecordBuffer_t recbuf;

	REC_EmptyBuffer(&recbuf);

	size_t i;
	for (i = 0; i < 3; ++i) {
		REC_PushRecord(&recbuf, &record);
		recbuf.records[i].isSent = true;
	}

	for (i = 3; i < BUFFER_LENGTH - 1; ++i) {
		REC_PushRecord(&recbuf, &record);
		recbuf.records[i].isSent = false;
	}

	size_t count = REC_CancelLastTransaction(&recbuf);
	TEST_ASSERT_EQUAL(3, count);

	size_t size = REC_GetSize(&recbuf);
	TEST_ASSERT_EQUAL(BUFFER_LENGTH - 1, size);

	for (i = 0; i <= size; ++i)	{
		TEST_ASSERT_EQUAL(0, recbuf.records[i].isSent);
	}
}

