#ifndef KEYBOARDTEST_H_INCLUDED
#define KEYBOARDTEST_H_INCLUDED

#include <cppunit/extensions/HelperMacros.h>

#define BUFF_LEN 80

class KeyboardTest : public CPPUNIT_NS::TestFixture
{
  CPPUNIT_TEST_SUITE( KeyboardTest );
  CPPUNIT_TEST( octave1Test );
 // CPPUNIT_TEST( octave2Test );
 // CPPUNIT_TEST( octave3Test );
 // CPPUNIT_TEST( octave4Test );
 // CPPUNIT_TEST( octave5Test );
 // CPPUNIT_TEST( octave6Test );
 // CPPUNIT_TEST( octave7Test );
  CPPUNIT_TEST_SUITE_END();

protected:
  int file_desc;
  char input[2];
  int ret_val;
  int ret_val_read;

  char read_buffer[BUFF_LEN];
  char write_buffer[BUFF_LEN];

public:
  void setUp();
  void tearDown();

protected:
  void octave1Test();
  void octave2Test();
  void octave3Test();
  void octave4Test();
  void octave5Test();
  void octave6Test();
  void octave7Test();

  void octaveTest(int octave);

  char playNote(int octave);
  void messageOrder(int octave);
  char correctNote(int octave, int i);

};

#endif // KEYBOARDTEST_H_INCLUDED
