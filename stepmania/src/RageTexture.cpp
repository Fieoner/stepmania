#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 File: RageTexture.h

 Desc: Abstract class for a texture with metadata.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
-----------------------------------------------------------------------------
*/

 
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "RageTexture.h"
#include "RageUtil.h"


//-----------------------------------------------------------------------------
// RageTexture constructor
//-----------------------------------------------------------------------------
RageTexture::RageTexture(
	RageDisplay* pScreen, 
	const CString &sFilePath, 
	DWORD dwMaxSize, 
	DWORD dwTextureColorDepth,
	int iMipMaps,
	int iAlphaBits,
	bool bDither,
	bool bStretch )
{
//	LOG->WriteLine( "RageTexture::RageTexture()" );

	// save a pointer to the D3D device
	m_pd3dDevice = pScreen->GetDevice();
	assert( m_pd3dDevice != NULL );

	// save the file path
	m_sFilePath = sFilePath;
//	m_pd3dTexture = NULL;
	m_iRefCount = 1;

	m_iSourceWidth = m_iSourceHeight = 0;
	m_iTextureWidth = m_iTextureHeight = 0;
	m_iImageWidth = m_iImageHeight = 0;
	m_iFramesWide = m_iFramesHigh = 1;
}

RageTexture::~RageTexture()
{

}


void RageTexture::CreateFrameRects()
{
	GetFrameDimensionsFromFileName( m_sFilePath, &m_iFramesWide, &m_iFramesHigh );

	///////////////////////////////////
	// Fill in the m_FrameRects with the bounds of each frame in the animation.
	///////////////////////////////////
	m_TextureCoordRects.RemoveAll();
	for( int j=0; j<m_iFramesHigh; j++ )		// traverse along Y
	{
		for( int i=0; i<m_iFramesWide; i++ )	// traverse along X (important that this is the inner loop)
		{
			FRECT frect( (i+0)/(float)m_iFramesWide*m_iImageWidth /(float)m_iTextureWidth,	// these will all be between 0.0 and 1.0
						 (j+0)/(float)m_iFramesHigh*m_iImageHeight/(float)m_iTextureHeight, 
						 (i+1)/(float)m_iFramesWide*m_iImageWidth /(float)m_iTextureWidth, 
						 (j+1)/(float)m_iFramesHigh*m_iImageHeight/(float)m_iTextureHeight );
			m_TextureCoordRects.Add( frect );	// the index of this array element will be (i + j*m_iFramesWide)
			
			//LOG->WriteLine( "Adding frect%d %f %f %f %f", (i + j*m_iFramesWide), frect.left, frect.top, frect.right, frect.bottom );
		}
	}
}

#include "string.h"
void RageTexture::GetFrameDimensionsFromFileName( CString sPath, int* piFramesWide, int* piFramesHigh ) const
{
	*piFramesWide = *piFramesHigh = 1;	// set default values in case we don't find the dimension in the file name

	sPath.MakeLower();

	CString sDir, sFName, sExt;
	splitrelpath( sPath, sDir, sFName, sExt);

	CStringArray arrayBits;
	split( sFName, " ", arrayBits, false );

	for( int i=0; i<arrayBits.GetSize(); i++ )
	{
		CString &sBit = arrayBits[ i ];	
		
		// Test to see if it looks like "%ux%u" (e.g. 16x8)

		CStringArray arrayDimensionsBits;
		split( sBit, "x", arrayDimensionsBits, false );

		if( arrayDimensionsBits.GetSize() != 2 )
			continue;
		else if( !IsAnInt(arrayDimensionsBits[0]) || !IsAnInt(arrayDimensionsBits[1]) )
			continue;

		*piFramesWide = atoi(arrayDimensionsBits[0]);
		*piFramesHigh = atoi(arrayDimensionsBits[1]);
		return;
	}

}

CString RageTexture::GetCacheFilePath()
{
	ULONG hash = GetHashForString( m_sFilePath );
	return ssprintf( "Cache\\%u.texinfo", hash );
}

const int FILE_CACHE_VERSION = 10;	// increment this when the cache file format changes

void RageTexture::SaveToCacheFile()
{
	LOG->WriteLine( "RageBitmapTexture::SaveToCacheFile()" );

	CString sCacheFilePath = GetCacheFilePath();

	FILE* file = fopen( sCacheFilePath, "w" );
	ASSERT( file != NULL );
	if( file == NULL )
		return;

	WriteIntToFile( file, FILE_CACHE_VERSION );
	WriteUlongToFile( file, GetHashForFile(m_sFilePath) );
	WriteStringToFile( file, m_sFilePath );
	WriteIntToFile( file, m_iSourceWidth );
	WriteIntToFile( file, m_iSourceHeight );
	WriteIntToFile( file, m_iTextureWidth );
	WriteIntToFile( file, m_iTextureHeight );
	WriteIntToFile( file, m_iImageWidth );
	WriteIntToFile( file, m_iImageHeight );
	WriteIntToFile( file, m_iFramesWide );
	WriteIntToFile( file, m_iFramesHigh );
	WriteIntToFile( file, m_TextureFormat );

	fclose( file );
}

bool RageTexture::LoadFromCacheFile()
{
	LOG->WriteLine( "Song::LoadFromCacheFile()" );

	CString sCacheFilePath = GetCacheFilePath();

	LOG->WriteLine( "cache file is '%s'.", sCacheFilePath );

	FILE* file = fopen( sCacheFilePath, "r" );
	if( file == NULL )
		return false;

	int iCacheVersion;
	ReadIntFromFile( file, iCacheVersion );
	if( iCacheVersion != FILE_CACHE_VERSION )
	{
		LOG->WriteLine( "Cache file versions don't match '%s'.", sCacheFilePath );
		fclose( file );
		return false;
	}

	ULONG hash;
	ReadUlongFromFile( file, hash );
	if( hash != GetHashForFile(m_sFilePath) )
	{
		LOG->WriteLine( "Cache file is out of date.", sCacheFilePath );
		fclose( file );
		return false;
	}

	ReadStringFromFile( file, m_sFilePath );
	ReadIntFromFile( file, m_iSourceWidth );
	ReadIntFromFile( file, m_iSourceHeight );
	ReadIntFromFile( file, m_iTextureWidth );
	ReadIntFromFile( file, m_iTextureHeight );
	ReadIntFromFile( file, m_iImageWidth );
	ReadIntFromFile( file, m_iImageHeight );
	ReadIntFromFile( file, m_iFramesWide );
	ReadIntFromFile( file, m_iFramesHigh );
	ReadIntFromFile( file, (int&)m_TextureFormat );

	fclose( file );
	return true;
}
