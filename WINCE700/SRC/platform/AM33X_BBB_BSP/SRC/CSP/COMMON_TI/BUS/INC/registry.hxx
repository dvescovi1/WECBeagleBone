// All rights reserved ADENEO EMBEDDED 2010
/*
================================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/

#pragma once
#include <string.hxx>

//------------------------------------------------------------------------------

namespace ce
{

//------------------------------------------------------------------------------

inline
LONG
RegQueryValue(
    HKEY hKey, 
    LPCWSTR name, 
    DWORD& value, 
    DWORD defaultValue = 0
    )
{
    LONG code;
    DWORD size = sizeof(DWORD);
    
    code = RegQueryValueEx(
        hKey, name, NULL, NULL, reinterpret_cast<BYTE*>(&value), &size
        );
    if (code != ERROR_SUCCESS) value = defaultValue;
    return code;
}

//------------------------------------------------------------------------------

inline
LONG
RegQueryValue(
    HKEY hKey, 
    LPCWSTR name, 
    DWORD *pValue, 
    DWORD defaultValue = 0
    )
{
    LONG code;
    DWORD size = sizeof(DWORD);
    
    code = RegQueryValueEx(
        hKey, name, NULL, NULL, reinterpret_cast<BYTE*>(pValue), &size
        );
    if (code != ERROR_SUCCESS) *pValue = defaultValue;
    
    return code;
}

//------------------------------------------------------------------------------

inline
LONG
RegQueryValueType(
    HKEY hKey, 
    LPCWSTR name, 
    DWORD& type
    )
{
    return RegQueryValueEx(hKey, name, NULL, &type, NULL, NULL);
}

//------------------------------------------------------------------------------

inline
LONG
RegQueryInfoKeySubKeys(
    HKEY hKey, 
    DWORD& count
    )
{
    return RegQueryInfoKey(
        hKey, NULL, NULL, NULL, &count, NULL, NULL, NULL, NULL, NULL, NULL, NULL
        );
}    

//------------------------------------------------------------------------------

template<unsigned _BUF_SIZE, class _Tr, class _Al>
inline
LONG
RegEnumKey(
    HKEY hKey, 
    DWORD index,
    _string_t<wchar_t, _BUF_SIZE, _Tr, _Al>& value
    )
{
    LONG code;
    DWORD size = 0;

    while ((code = RegEnumKeyEx(
        hKey, index, static_cast<LPWSTR>(value.get_buffer()),
        &(size = value.capacity()), NULL, NULL, NULL, NULL
        )) == ERROR_MORE_DATA)
        {
        if (!value.reserve(size + 1)) break;
        }

    if (code == ERROR_SUCCESS)
        {
        value.resize(size);
        }
    
    return code;
}

//------------------------------------------------------------------------------

template<unsigned _BUF_SIZE, class _Tr, class _Al>
inline
LONG
RegEnumValue(
    HKEY hKey,
    DWORD index,
    _string_t<wchar_t, _BUF_SIZE, _Tr, _Al>& valueName,
    DWORD *pType, 
    _string_t<wchar_t, _BUF_SIZE, _Tr, _Al>& valueData
    )
{
    LONG code;
    DWORD nameSize = 0;
    DWORD dataSize = 0;


    while ((code = RegEnumValue(
            hKey, index, 
            static_cast<LPWSTR>(valueName.get_buffer()),
            &(nameSize = valueName.capacity()),
            NULL, pType, 
            (UCHAR*)static_cast<LPWSTR>(valueData.get_buffer()),
            &(dataSize = valueData.capacity()))) == ERROR_MORE_DATA)
        {
        if (!valueName.reserve(nameSize)) break;
        if (!valueData.reserve(dataSize)) break;
        }

    if (code == ERROR_SUCCESS)
        {
        valueName.resize(nameSize);
        valueData.resize(dataSize);
        }
    
    return code;
}

};

//------------------------------------------------------------------------------

