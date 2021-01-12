#ifndef RWTEST_H_INCLUDED
#define RWTEST_H_INCLUDED

#include <cppunit/extensions/HelperMacros.h>

#define BUFF_LEN 80

class RWTest : public CPPUNIT_NS::TestFixture
{
  CPPUNIT_TEST_SUITE( RWTest );
  CPPUNIT_TEST( read_write );
  CPPUNIT_TEST_SUITE_END();

protected:
  int file_desc;

public:
  void setUp();
  void tearDown();

protected:
  void read_write(); //test desired read/write of every stepper with every note

};

#endif // RWTEST_H_INCLUDED
