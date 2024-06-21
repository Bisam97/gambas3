/***************************************************************************

  gb_constant_temp.h

  (c) Benoît Minisini <benoit.minisini@gambas-basic.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
  MA 02110-1301, USA.

***************************************************************************/

	GB_CONSTANT("Binary", "i", GB_COMP_BINARY),
	GB_CONSTANT("IgnoreCase", "i", GB_COMP_NOCASE),
	GB_CONSTANT("Language", "i", GB_COMP_LANG),
	GB_CONSTANT("Like", "i", GB_COMP_LIKE),
	GB_CONSTANT("Match", "i", GB_COMP_MATCH),
	GB_CONSTANT("Natural", "i", GB_COMP_NATURAL),

	GB_CONSTANT("Ascent", "i", GB_COMP_ASCENT),
	GB_CONSTANT("Descent", "i", GB_COMP_DESCENT),

	GB_CONSTANT("Null", "i", GB_T_NULL),
	GB_CONSTANT("Boolean", "i", GB_T_BOOLEAN),
	GB_CONSTANT("Byte", "i", GB_T_BYTE),
	GB_CONSTANT("Short", "i", GB_T_SHORT),
	GB_CONSTANT("Integer", "i", GB_T_INTEGER),
	GB_CONSTANT("Long", "i", GB_T_LONG),
	GB_CONSTANT("Float", "i", GB_T_FLOAT),
	GB_CONSTANT("Single", "i", GB_T_SINGLE),
	GB_CONSTANT("Date", "i", GB_T_DATE),
	GB_CONSTANT("String", "i" , GB_T_STRING),
	GB_CONSTANT("Pointer", "i" , GB_T_POINTER),
	GB_CONSTANT("Function", "i" , GB_T_FUNCTION),
	GB_CONSTANT("Variant", "i", GB_T_VARIANT),
	GB_CONSTANT("Class", "i" , GB_T_CLASS),
	GB_CONSTANT("Object", "i", GB_T_OBJECT),

	GB_CONSTANT("File", "i", GB_STAT_FILE),
	GB_CONSTANT("Directory", "i", GB_STAT_DIRECTORY),
	GB_CONSTANT("Device", "i", GB_STAT_DEVICE),
	GB_CONSTANT("Pipe", "i", GB_STAT_PIPE),
	GB_CONSTANT("Socket", "i", GB_STAT_SOCKET),
	GB_CONSTANT("Link", "i", GB_STAT_LINK),

	GB_CONSTANT("Standard", "i", GB_LF_STANDARD),
	GB_CONSTANT("GeneralNumber", "i", GB_LF_GENERAL_NUMBER),
	GB_CONSTANT("ShortNumber", "i", GB_LF_SHORT_NUMBER),
	GB_CONSTANT("Fixed", "i", GB_LF_FIXED),
	GB_CONSTANT("Percent", "i", GB_LF_PERCENT),
	GB_CONSTANT("Scientific", "i", GB_LF_SCIENTIFIC),
	GB_CONSTANT("Currency", "i", GB_LF_CURRENCY),
	GB_CONSTANT("International", "i", GB_LF_INTERNATIONAL),
	GB_CONSTANT("GeneralDate", "i", GB_LF_GENERAL_DATE),
	GB_CONSTANT("LongDate", "i", GB_LF_LONG_DATE),
	GB_CONSTANT("MediumDate", "i", GB_LF_MEDIUM_DATE),
	GB_CONSTANT("ShortDate", "i", GB_LF_SHORT_DATE),
	GB_CONSTANT("LongTime", "i", GB_LF_LONG_TIME),
	GB_CONSTANT("MediumTime", "i", GB_LF_MEDIUM_TIME),
	GB_CONSTANT("ShortTime", "i", GB_LF_SHORT_TIME),

	GB_CONSTANT("Read", "i", GB_ST_READ),
	GB_CONSTANT("Write", "i", GB_ST_WRITE),
	GB_CONSTANT("Exec", "i", GB_ST_EXEC),
	GB_CONSTANT("Input", "i", GB_ST_READ + GB_ST_BUFFERED),
	GB_CONSTANT("Output", "i", GB_ST_WRITE + GB_ST_BUFFERED),

	GB_CONSTANT("Sunday", "i", 0),
	GB_CONSTANT("Monday", "i", 1),
	GB_CONSTANT("Tuesday", "i", 2),
	GB_CONSTANT("Wednesday", "i", 3),
	GB_CONSTANT("Thursday", "i", 4),
	GB_CONSTANT("Friday", "i", 5),
	GB_CONSTANT("Saturday", "i", 6),

	GB_CONSTANT("Millisecond", "i", GB_DP_MILLISECOND),
	GB_CONSTANT("Second", "i", GB_DP_SECOND),
	GB_CONSTANT("Minute", "i", GB_DP_MINUTE),
	GB_CONSTANT("Hour", "i", GB_DP_HOUR),
	GB_CONSTANT("Day", "i", GB_DP_DAY),
	GB_CONSTANT("WeekDay", "i", GB_DP_WEEKDAY),
	GB_CONSTANT("Week", "i", GB_DP_WEEK),
	GB_CONSTANT("Month", "i", GB_DP_MONTH),
	GB_CONSTANT("Quarter", "i", GB_DP_QUARTER),
	GB_CONSTANT("Year", "i", GB_DP_YEAR),

	GB_CONSTANT("LittleEndian", "i", GB_LITTLE_ENDIAN),
	GB_CONSTANT("BigEndian", "i", GB_BIG_ENDIAN),

	GB_CONSTANT("Unix", "i", GB_EOL_UNIX),
	GB_CONSTANT("Windows", "i", GB_EOL_WINDOWS),
	GB_CONSTANT("Mac", "i", GB_EOL_MAC),
