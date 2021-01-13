#ifndef INVALIDINPUTTEST_H_INCLUDED
#define INVALIDINPUTTEST_H_INCLUDED

#include <cppunit/extensions/HelperMacros.h>

#define MAX_STEPPERS 4
#define BUFF_LEN 80

class InvalidInputTest : public CPPUNIT_NS::TestFixture
{
  CPPUNIT_TEST_SUITE( InvalidInputTest );
  CPPUNIT_TEST( invalidStepperTest );
  CPPUNIT_TEST( invalidNoteUnderTest );
  CPPUNIT_TEST( invalidNoteOverTest );
  CPPUNIT_TEST( invalidAllTest);
  CPPUNIT_TEST_SUITE_END();

protected:
  int file_desc;
  char input[2];
  int ret_val_write, ret_val_read;
  char read_buffer[BUFF_LEN];

public:
  void setUp();
  void tearDown();

protected:
  void invalidStepperTest();
  void invalidNoteUnderTest();
  void invalidNoteOverTest();
  void invalidAllTest();

};

#endif // INVALIDINPUTTEST_H_INCLUDED
