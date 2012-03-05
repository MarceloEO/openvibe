#ifndef __OpenViBE_AcquisitionServer_CConfigurationEmokitEPOC_H__
#define __OpenViBE_AcquisitionServer_CConfigurationEmokitEPOC_H__

#if defined TARGET_HAS_ThirdPartyEmokitAPI

#include "../ovasCConfigurationBuilder.h"
#include "../ovasIDriver.h"
#include "../ovasCHeader.h"

#include <gtk/gtk.h>

namespace OpenViBEAcquisitionServer
{
	/**
	 * \class CConfigurationEmokitEPOC
	 * \author Kyle Machulis
	 * \date 4 march 2012
	 * \brief The CConfigurationEmokitEPOC handles the configuration dialog specific to the Emotiv EPOC headset, running thru the Emokit driver.
	 *
	 * \sa CDriverEmokitEPOC
	 */
	class CConfigurationEmokitEPOC : public OpenViBEAcquisitionServer::CConfigurationBuilder
	{
	public:

		CConfigurationEmokitEPOC(OpenViBEAcquisitionServer::IDriverContext& rDriverContext, const char* sGtkBuilderFileName, OpenViBE::boolean& rUseGyroscope);

		virtual OpenViBE::boolean preConfigure(void);
		virtual OpenViBE::boolean postConfigure(void);

	protected:

		OpenViBEAcquisitionServer::IDriverContext& m_rDriverContext;
		OpenViBE::boolean& m_rUseGyroscope;
	};

};

#endif // TARGET_HAS_ThirdPartyEmokitAPI

#endif // __OpenViBE_AcquisitionServer_CConfigurationEmokitEPOC_H__
