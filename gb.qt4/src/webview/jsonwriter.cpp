/***************************************************************************

	jsonwriter.cpp

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


#include <cmath>
#include <qglobal.h>
#include <qlocale.h>

#include "jsonwriter.h"

static inline uchar hexa_digit(uchar c)
{
	return (c < 0xa ? '0' + c : 'a' + c - 0xa);
}

static QByteArray escapedString(const QString &s)
{
	QByteArray utf8 = s.toUtf8();
	QByteArray ba;
	int i;
	
	for (i = 0; i < utf8.size(); i++)
	{
		uchar c = utf8.at(i);
		if (c < 0x20 || c == 0x22 || c == 0x5C)
		{
			ba += '\\';
			switch (c) 
			{
				case 0x22: ba += '"'; break;
				case 0x5c: ba += '\\'; break;
				case 0x8:  ba += 'b'; break;
				case 0xc:  ba += 'f'; break;
				case 0xa:  ba += 'n'; break;
				case 0xd:  ba += 'r'; break;
				case 0x9:  ba += 't'; break;
				default:
					ba += "u00";
					ba += hexa_digit(c >> 4);
					ba += hexa_digit(c & 0xf);
			}
		}
		ba += c;
	}
	
	return ba;
}

static void arrayContentToJson(const QVariantList &v, QByteArray &json)
{
	uint i;
	uint size = v.size();
	
	if (size == 0)
		return;

	size--;
	for(i = 0; i < size; i++)
	{
		JSONWRITER_valueToJson(v.at(i), json);
		json += ',';
	}
	
	JSONWRITER_valueToJson(v.at(i), json);
}

static void objectContentToJson(const QVariantMap &v, QByteArray &json)
{
	uint size = v.size();
	bool comma;
	
	if (size == 0)
		return;
	
	comma = false;
	QMapIterator<QString, QVariant> i(v);
	while (i.hasNext()) 
	{
		i.next();
		if (comma)
			json += ',';
		else
			comma = true;
		json += '"';
		json += escapedString(i.key());
		json += "\":";
		JSONWRITER_valueToJson(i.value(), json);
	}
}

void JSONWRITER_valueToJson(const QVariant &v, QByteArray &json)
{
	if (v.isNull())
	{
		json += "null";
		return;
	}

	switch (v.type())
	{
		case QVariant::Bool:
			json += v.toBool() ? "true" : "false";
			break;
    case QVariant::ULongLong:
			json += QByteArray::number(v.value<qulonglong>());
		case QVariant::UInt:
			json += QByteArray::number(v.value<quint32>());
		case QVariant::Double:
		{
			const double d = v.toDouble();
			if (std::isnan(d) || std::isinf(d))
			{
				json += "null";
			}
			else
			{ 
				QString str = QByteArray::number(d, 'g', 12);
        /*if (!str.contains( '.' ) && !str.contains('e'))
          str += ".0";*/
				json += str;
			} 
			break;
		}
		case QVariant::String:
			json += '"';
			json += escapedString(v.toString());
			json += '"';
			break;
		case QVariant::List:
			json += '[';
			arrayContentToJson(v.toList(), json);
			json += ']';
			break;
		case QVariant::Map:
			json += '{';
			objectContentToJson(v.toMap(), json);
			json += '}';
			break;
		default:
			json += "undefined";
	}
}
