#include <cstdlib>

#include <shared/Logging.h>

#include "keyvalues/Keyvalues.h"

#include "CCmdLineConfig.h"

#define CMDLINESETTINGS_NAME_KEY "name"
#define CMDLINESETTINGS_PARAMS_BLOCK "parameters"
#define CMDLINESETTINGS_COPYOUTPUTFILES_KEY "copyOutputFiles"
#define CMDLINESETTINGS_FILTERS_BLOCK "filters"
#define CMDLINESETTINGS_FILTER_KEY "filter"

namespace settings
{
const char* const CCmdLineConfig::IO_BLOCK_NAME = "settings";

CCmdLineConfig::CCmdLineConfig( const std::string& szName, const Parameters_t& parameters, const bool bCopyOutputFiles, const Filters_t& filters )
	: m_szName( szName )
	, m_Parameters( parameters )
	, m_bCopyOutputFiles( bCopyOutputFiles )
	, m_Filters( filters )
{
}

CCmdLineConfig::CCmdLineConfig( std::string&& szName, Parameters_t&& parameters, const bool bCopyOutputFiles, Filters_t&& filters )
	: m_szName( std::move( szName ) )
	, m_Parameters( std::move( parameters ) )
	, m_bCopyOutputFiles( bCopyOutputFiles )
	, m_Filters( std::move( filters ) )
{
}

CCmdLineConfig::CCmdLineConfig( const std::string& szName )
	: m_szName( szName )
{
}

CCmdLineConfig::CCmdLineConfig( std::string&& szName )
	: m_szName( std::move( szName ) )
{
}

std::shared_ptr<CCmdLineConfig> LoadCmdLineConfig( const kv::Block& kvSettings )
{
	auto name = kvSettings.FindFirstChild<kv::KV>( CMDLINESETTINGS_NAME_KEY );
	auto params = kvSettings.FindFirstChild<kv::Block>( CMDLINESETTINGS_PARAMS_BLOCK );
	auto shouldCopyFiles = kvSettings.FindFirstChild<kv::KV>( CMDLINESETTINGS_COPYOUTPUTFILES_KEY );
	auto filters = kvSettings.FindFirstChild<kv::Block>( CMDLINESETTINGS_FILTERS_BLOCK );

	if( !name || !params || !shouldCopyFiles || !filters )
		return nullptr;

	CCmdLineConfig::Parameters_t parameters;

	for( const auto& child : params->GetChildren() )
	{
		if( auto param = dynamic_cast<kv::KV*>( child ) )
		{
			parameters.emplace_back( std::make_pair( param->GetKey().CStr(), param->GetValue().CStr() ) );
		}
		else
		{
			Warning( "Command line parameter has invalid format!\n" );
		}
	}

	const bool bCopyOutputFiles = atoi( shouldCopyFiles->GetValue().CStr() ) != 0;

	CCmdLineConfig::Filters_t filterList;

	for( const auto& child : filters->GetChildren() )
	{
		if( auto filter = dynamic_cast<kv::KV*>( child ) )
		{
			filterList.emplace_back( filter->GetValue().CStr() );
		}
		else
		{
			Warning( "Command line filter has invalid format!\n" );
		}
	}

	return std::make_shared<CCmdLineConfig>( name->GetValue().CStr(), std::move( parameters ), bCopyOutputFiles, std::move( filterList ) );
}

bool SaveCmdLineConfig( const CCmdLineConfig& settings, kv::Writer& writer )
{
	writer.BeginBlock( CCmdLineConfig::IO_BLOCK_NAME );
	writer.WriteKeyvalue( CMDLINESETTINGS_NAME_KEY, settings.GetName().c_str() );

	writer.BeginBlock( CMDLINESETTINGS_PARAMS_BLOCK );

	for( const auto& param : settings.GetParameters() )
	{
		writer.WriteKeyvalue( param.first.c_str(), param.second.c_str() );
	}

	writer.EndBlock();

	writer.WriteKeyvalue( CMDLINESETTINGS_COPYOUTPUTFILES_KEY, settings.ShouldCopyOutputFiles() ? "1" : "0" );

	writer.BeginBlock( CMDLINESETTINGS_FILTERS_BLOCK );

	for( const auto& filter : settings.GetFilters() )
	{
		writer.WriteKeyvalue( CMDLINESETTINGS_FILTER_KEY, filter.c_str() );
	}

	writer.EndBlock();

	writer.EndBlock();

	return !writer.ErrorOccurred();
}
}