#ifndef DELAYTEST_H_INCLUDED
#define DELAYTEST_H_INCLUDED

#include <cppunit/extensions/HelperMacros.h>

#define BUFF_LEN 80

class DelayTest : public CPPUNIT_NS::TestFixture
{
  CPPUNIT_TEST_SUITE( DelayTest );
  CPPUNIT_TEST( delayTest );
  CPPUNIT_TEST_SUITE_END();

protected:
  int file_desc;

public:
  void setUp();
  void tearDown();

protected:
  void delayTest(); //test delay of writing to memory

};

#endif // DELAYTEST_H_INCLUDED