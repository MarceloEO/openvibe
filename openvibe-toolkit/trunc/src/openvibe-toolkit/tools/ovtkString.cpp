#include "ovtkString.h"

#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <cctype>

#include <cstring>

using namespace OpenViBE;

namespace OpenViBEToolkit
{
	namespace Tools
	{
		namespace String
		{
			namespace
			{
				inline boolean isSeparator(uint8 ui8Value, uint8* pSeparator, uint32 ui32SeparatorCount)
				{
					for(uint32 i=0; i<ui32SeparatorCount; i++)
					{
						if(ui8Value==pSeparator[i])
						{
							return true;
						}
					}
					return false;
				}

				// because std::tolower has multiple signatures,
				// it can not be easily used in std::transform
				// this workaround is taken from http://www.gcek.net/ref/books/sw/cpp/ticppv2/
				template <class charT>
				charT to_lower(charT c)
				{
					return std::tolower(c);
				}
			}
		};
	};
};

uint32 OpenViBEToolkit::Tools::String::split(const CString& rString, CString* pString, uint32 ui32StringCount, uint8 ui8Separator)
{
	return OpenViBEToolkit::Tools::String::split(rString, pString, ui32StringCount, &ui8Separator, 1);
}

uint32 OpenViBEToolkit::Tools::String::split(const CString& rString, CString* pString, uint32 ui32StringCount, uint8* pSeparator, uint32 ui32SeparatorCount)
{
	if(pString==NULL) return 0;
	if(ui32StringCount==0) return 0;
	if(ui32SeparatorCount==0 || pSeparator==NULL) return 0;

	std::string l_sString(rString.toASCIIString());
	std::vector < std::string > l_vResult;
	std::string::size_type i=0;
	std::string::size_type j=0;
	while(i<l_sString.length())
	{
		j=i;
		while(j<l_sString.length() && !OpenViBEToolkit::Tools::String::isSeparator(l_sString[j], pSeparator, ui32SeparatorCount))
		{
			j++;
		}
//		if(i!=j)
		{
			l_vResult.push_back(std::string(l_sString, i, j-i));
		}
		i=j+1;
	}
	if(l_sString.length()!=0 && OpenViBEToolkit::Tools::String::isSeparator(l_sString[l_sString.length()-1], pSeparator, ui32SeparatorCount))
	{
		l_vResult.push_back("");
	}

	if(l_vResult.size()>ui32StringCount) return 0;

	for(uint32 i=0; i<l_vResult.size(); i++)
	{
		pString[i]=l_vResult[i].c_str();
	}
	return l_vResult.size();
}


boolean OpenViBEToolkit::Tools::String::isAlmostEqual(const CString& rString1, const CString& rString2, const boolean bCaseSensitive, const boolean bRemoveStartSpaces, const boolean bRemoveEndSpaces)
{
	const char* l_pString1_start=rString1.toASCIIString();
	const char* l_pString1_end=l_pString1_start+::strlen(l_pString1_start)-1;

	const char* l_pString2_start=rString2.toASCIIString();
	const char* l_pString2_end=l_pString2_start+::strlen(l_pString2_start)-1;

	if(bRemoveStartSpaces)
	{
		while(*l_pString1_start==' ') l_pString1_start++;
		while(*l_pString2_start==' ') l_pString2_start++;
	}

	if(bRemoveEndSpaces)
	{
		while(l_pString1_start<l_pString1_end && *l_pString1_end==' ') l_pString1_end--;
		while(l_pString2_start<l_pString2_end && *l_pString2_end==' ') l_pString2_end--;
	}

	std::string l_sString1(l_pString1_start, l_pString1_end-l_pString1_start+1);
	std::string l_sString2(l_pString2_start, l_pString2_end-l_pString2_start+1);

	if(!bCaseSensitive)
	{
		std::transform(l_sString1.begin(), l_sString1.end(), l_sString1.begin(), to_lower<std::string::value_type>);
		std::transform(l_sString2.begin(), l_sString2.end(), l_sString2.begin(), to_lower<std::string::value_type>);
	}

	return l_sString1==l_sString2;
}
