/* FasTC
 * Copyright (c) 2014 University of North Carolina at Chapel Hill.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for educational, research, and non-profit purposes, without
 * fee, and without a written agreement is hereby granted, provided that the
 * above copyright notice, this paragraph, and the following four paragraphs
 * appear in all copies.
 *
 * Permission to incorporate this software into commercial products may be
 * obtained by contacting the authors or the Office of Technology Development
 * at the University of North Carolina at Chapel Hill <otd@unc.edu>.
 *
 * This software program and documentation are copyrighted by the University of
 * North Carolina at Chapel Hill. The software program and documentation are
 * supplied "as is," without any accompanying services from the University of
 * North Carolina at Chapel Hill or the authors. The University of North
 * Carolina at Chapel Hill and the authors do not warrant that the operation of
 * the program will be uninterrupted or error-free. The end-user understands
 * that the program was developed for research purposes and is advised not to
 * rely exclusively on the program for any reason.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE
 * AUTHORS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE USE OF
 * THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF NORTH CAROLINA
 * AT CHAPEL HILL OR THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS SPECIFICALLY
 * DISCLAIM ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND ANY 
 * STATUTORY WARRANTY OF NON-INFRINGEMENT. THE SOFTWARE PROVIDED HEREUNDER IS ON
 * AN "AS IS" BASIS, AND THE UNIVERSITY  OF NORTH CAROLINA AT CHAPEL HILL AND
 * THE AUTHORS HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, 
 * ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Please send all BUG REPORTS to <pavel@cs.unc.edu>.
 *
 * The authors may be contacted via:
 *
 * Pavel Krajcevski
 * Dept of Computer Science
 * 201 S Columbia St
 * Frederick P. Brooks, Jr. Computer Science Bldg
 * Chapel Hill, NC 27599-3175
 * USA
 * 
 * <http://gamma.cs.unc.edu/FasTC/>
 */

#include "gtest/gtest.h"
#include "IntegerEncoding.h"
using ASTCC::IntegerEncodedValue;

TEST(IntegerEncoding, GetEncoding) {
  // According to table C.2.7
  IntegerEncodedValue val = IntegerEncodedValue::CreateEncoding(1);
  EXPECT_EQ(val.GetEncoding(), ASTCC::eIntegerEncoding_JustBits);
  EXPECT_EQ(val.BaseBitLength(), 1);

  val = IntegerEncodedValue::CreateEncoding(2);
  EXPECT_EQ(val.GetEncoding(), ASTCC::eIntegerEncoding_Trit);
  EXPECT_EQ(val.BaseBitLength(), 0);

  val = IntegerEncodedValue::CreateEncoding(3);
  EXPECT_EQ(val.GetEncoding(), ASTCC::eIntegerEncoding_JustBits);
  EXPECT_EQ(val.BaseBitLength(), 2);

  val = IntegerEncodedValue::CreateEncoding(4);
  EXPECT_EQ(val.GetEncoding(), ASTCC::eIntegerEncoding_Quint);
  EXPECT_EQ(val.BaseBitLength(), 0);

  val = IntegerEncodedValue::CreateEncoding(5);
  EXPECT_EQ(val.GetEncoding(), ASTCC::eIntegerEncoding_Trit);
  EXPECT_EQ(val.BaseBitLength(), 1);

  val = IntegerEncodedValue::CreateEncoding(7);
  EXPECT_EQ(val.GetEncoding(), ASTCC::eIntegerEncoding_JustBits);
  EXPECT_EQ(val.BaseBitLength(), 3);

  val = IntegerEncodedValue::CreateEncoding(9);
  EXPECT_EQ(val.GetEncoding(), ASTCC::eIntegerEncoding_Quint);
  EXPECT_EQ(val.BaseBitLength(), 1);

  val = IntegerEncodedValue::CreateEncoding(11);
  EXPECT_EQ(val.GetEncoding(), ASTCC::eIntegerEncoding_Trit);
  EXPECT_EQ(val.BaseBitLength(), 2);

  val = IntegerEncodedValue::CreateEncoding(15);
  EXPECT_EQ(val.GetEncoding(), ASTCC::eIntegerEncoding_JustBits);
  EXPECT_EQ(val.BaseBitLength(), 4);

  val = IntegerEncodedValue::CreateEncoding(19);
  EXPECT_EQ(val.GetEncoding(), ASTCC::eIntegerEncoding_Quint);
  EXPECT_EQ(val.BaseBitLength(), 2);

  val = IntegerEncodedValue::CreateEncoding(23);
  EXPECT_EQ(val.GetEncoding(), ASTCC::eIntegerEncoding_Trit);
  EXPECT_EQ(val.BaseBitLength(), 3);

  val = IntegerEncodedValue::CreateEncoding(31);
  EXPECT_EQ(val.GetEncoding(), ASTCC::eIntegerEncoding_JustBits);
  EXPECT_EQ(val.BaseBitLength(), 5);
}

