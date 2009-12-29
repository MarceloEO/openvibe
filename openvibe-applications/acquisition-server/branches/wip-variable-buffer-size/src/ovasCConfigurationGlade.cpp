#include "ovasCConfigurationGlade.h"
#include "ovasIHeader.h"

#include <openvibe-toolkit/ovtk_all.h>

#include <iostream>
#include <fstream>
#include <list>
#include <stdlib.h>

#define OVAS_ElectrodeNames_File           "../share/openvibe-applications/acquisition-server/electrode-names.txt"
#define OVAS_ConfigureGUIElectrodes_File   "../share/openvibe-applications/acquisition-server/interface-channel-names.glade"

using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace std;

//___________________________________________________________________//
//                                                                   //

static void gtk_combo_box_set_active_text(::GtkComboBox* pComboBox, const gchar* sActiveText)
{
	::GtkTreeModel* l_pTreeModel=gtk_combo_box_get_model(pComboBox);
	::GtkTreeIter itComboEntry;
	int l_iIndex=0;
	gchar* l_sComboEntryName=NULL;
	if(gtk_tree_model_get_iter_first(l_pTreeModel, &itComboEntry))
	{
		do
		{
			gtk_tree_model_get(l_pTreeModel, &itComboEntry, 0, &l_sComboEntryName, -1);
			if(string(l_sComboEntryName)==string(sActiveText))
			{
				gtk_combo_box_set_active(pComboBox, l_iIndex);
				return;
			}
			else
			{
				l_iIndex++;
			}
		}
		while(gtk_tree_model_iter_next(l_pTreeModel, &itComboEntry));
	}
}

static void button_change_channel_names_cb(::GtkButton* pButton, void* pUserData)
{
#if defined _DEBUG_Callbacks_
	cout << "button_change_channel_names_cb" << endl;
#endif
	static_cast<CConfigurationGlade*>(pUserData)->buttonChangeChannelNamesCB(pButton);
}

static void button_apply_channel_name_cb(::GtkButton* pButton, void* pUserData)
{
#if defined _DEBUG_Callbacks_
	cout << "button_apply_channel_name_cb" << endl;
#endif
	static_cast<CConfigurationGlade*>(pUserData)->buttonApplyChannelNameCB(pButton);
}

static void button_remove_channel_name_cb(::GtkButton* pButton, void* pUserData)
{
#if defined _DEBUG_Callbacks_
	cout << "button_remove_channel_name_cb" << endl;
#endif
	static_cast<CConfigurationGlade*>(pUserData)->buttonRemoveChannelNameCB(pButton);
}

//___________________________________________________________________//
//                                                                   //

CConfigurationGlade::CConfigurationGlade(const char* sGladeXMLFileName)
	:m_pGladeConfigureInterface(NULL)
	,m_pElectrodeNameListStore(NULL)
	,m_pChannelNameListStore(NULL)
	,m_sGladeXMLFileName(sGladeXMLFileName?sGladeXMLFileName:"")
	,m_sElectrodeFileName(OVAS_ElectrodeNames_File)
	,m_sGladeXMLChannelsFileName(OVAS_ConfigureGUIElectrodes_File)
	,m_pHeader(NULL)
{
}

CConfigurationGlade::~CConfigurationGlade(void)
{
}

//___________________________________________________________________//
//                                                                   //

boolean CConfigurationGlade::configure(IHeader& rHeader)
{
	m_bApplyConfiguration=false;

	m_pHeader=&rHeader;
	this->preConfigure();
	m_bApplyConfiguration=this->doConfigure();
	this->postConfigure();
	m_pHeader=NULL;

	return m_bApplyConfiguration;
}

boolean CConfigurationGlade::preConfigure(void)
{
	// Prepares interface

	m_pGladeConfigureInterface=glade_xml_new(m_sGladeXMLFileName.c_str(), NULL, NULL);
	m_pGladeConfigureChannelInterface=glade_xml_new(m_sGladeXMLChannelsFileName.c_str(), NULL, NULL);

	// Finds all the widgets

	m_pDialog=glade_xml_get_widget(m_pGladeConfigureInterface, "openvibe-acquisition-server-settings");

	m_pIdentifier=glade_xml_get_widget(m_pGladeConfigureInterface, "spinbutton_identifier");
	m_pAge=glade_xml_get_widget(m_pGladeConfigureInterface, "spinbutton_age");
	m_pNumberOfChannels=glade_xml_get_widget(m_pGladeConfigureInterface, "spinbutton_number_of_channels");
	m_pSamplingFrequency=glade_xml_get_widget(m_pGladeConfigureInterface, "combobox_sampling_frequency");
	m_pGender=glade_xml_get_widget(m_pGladeConfigureInterface, "combobox_gender");

	m_pElectrodeNameTreeView=glade_xml_get_widget(m_pGladeConfigureChannelInterface, "treeview_electrode_names");
	m_pChannelNameTreeView=glade_xml_get_widget(m_pGladeConfigureChannelInterface, "treeview_channel_names");

	// Prepares electrode name tree view

	::GtkTreeView* l_pElectrodeNameTreeView=GTK_TREE_VIEW(m_pElectrodeNameTreeView);
	::GtkCellRenderer* l_pElectrodeNameIndexCellRenderer=gtk_cell_renderer_text_new();
	::GtkTreeViewColumn* l_pElectrodeNameIndexTreeViewColumn=gtk_tree_view_column_new_with_attributes("Name", l_pElectrodeNameIndexCellRenderer, "text", 0, NULL);

	gtk_tree_view_append_column(l_pElectrodeNameTreeView, l_pElectrodeNameIndexTreeViewColumn);

	// Prepares channel name tree view

	::GtkTreeView* l_pChannelNameTreeView=GTK_TREE_VIEW(m_pChannelNameTreeView);
	::GtkCellRenderer* l_pChannelNameIndexCellRenderer=gtk_cell_renderer_text_new();
	::GtkCellRenderer* l_pChannelNameValueCellRenderer=gtk_cell_renderer_text_new();
	::GtkTreeViewColumn* l_pChannelNameIndexTreeViewColumn=gtk_tree_view_column_new_with_attributes("Index", l_pChannelNameIndexCellRenderer, "text", 0, NULL);
	::GtkTreeViewColumn* l_pChannelNameValueTreeViewColumn=gtk_tree_view_column_new_with_attributes("Name", l_pChannelNameValueCellRenderer, "text", 1, NULL);

	gtk_tree_view_append_column(l_pChannelNameTreeView, l_pChannelNameIndexTreeViewColumn);
	gtk_tree_view_append_column(l_pChannelNameTreeView, l_pChannelNameValueTreeViewColumn);

	// Connects custom GTK signals

	g_signal_connect(glade_xml_get_widget(m_pGladeConfigureInterface,        "button_change_channel_names"), "pressed", G_CALLBACK(button_change_channel_names_cb), this);
	g_signal_connect(glade_xml_get_widget(m_pGladeConfigureChannelInterface, "button_apply_channel_name"),   "pressed", G_CALLBACK(button_apply_channel_name_cb),   this);
	g_signal_connect(glade_xml_get_widget(m_pGladeConfigureChannelInterface, "button_remove_channel_name"),  "pressed", G_CALLBACK(button_remove_channel_name_cb),  this);
	glade_xml_signal_autoconnect(m_pGladeConfigureInterface);
	glade_xml_signal_autoconnect(m_pGladeConfigureChannelInterface);

	// Configures interface with preconfigured values

	if(m_pHeader->isExperimentIdentifierSet())
	{
		gtk_spin_button_set_value(
			GTK_SPIN_BUTTON(m_pIdentifier),
			m_pHeader->getExperimentIdentifier());
	}
	if(m_pHeader->isSubjectAgeSet())
	{
		gtk_spin_button_set_value(
			GTK_SPIN_BUTTON(m_pAge),
			m_pHeader->getSubjectAge());
	}
	if(m_pHeader->isChannelCountSet())
	{
		gtk_spin_button_set_value(
			GTK_SPIN_BUTTON(m_pNumberOfChannels),
			m_pHeader->getChannelCount());
		for(uint32 i=0; i<m_pHeader->getChannelCount(); i++)
		{
			m_vChannelName[i]=m_pHeader->getChannelName(i);
		}
	}
	if(m_pHeader->isSamplingFrequencySet())
	{
		char l_sSamplingFrequency[1024];
		sprintf(l_sSamplingFrequency, "%i", (int)m_pHeader->getSamplingFrequency());
		gtk_combo_box_set_active_text(
			GTK_COMBO_BOX(m_pSamplingFrequency),
			l_sSamplingFrequency);
	}
	else
	{
		gtk_combo_box_set_active(
			GTK_COMBO_BOX(m_pSamplingFrequency),
			0);
	}
	if(m_pHeader->isSubjectGenderSet())
	{
		gtk_combo_box_set_active_text(
			GTK_COMBO_BOX(m_pGender),
				m_pHeader->getSubjectGender()==OVTK_Value_Gender_Male?"male":
				m_pHeader->getSubjectGender()==OVTK_Value_Gender_Female?"female":
				m_pHeader->getSubjectGender()==OVTK_Value_Gender_Unknown?"unknown":
				"unspecified");
	}
	else
	{
		gtk_combo_box_set_active(
			GTK_COMBO_BOX(m_pGender),
			0);
	}

	return true;
}

boolean CConfigurationGlade::doConfigure(void)
{
	return gtk_dialog_run(GTK_DIALOG(m_pDialog))==GTK_RESPONSE_APPLY;
}

boolean CConfigurationGlade::postConfigure(void)
{
	if(m_bApplyConfiguration)
	{
		string l_sGender=gtk_combo_box_get_active_text(GTK_COMBO_BOX(m_pGender));
		m_pHeader->setExperimentIdentifier(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(m_pIdentifier)));
		m_pHeader->setSubjectAge(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(m_pAge)));
		m_pHeader->setChannelCount(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(m_pNumberOfChannels)));
		m_pHeader->setSamplingFrequency(atoi(gtk_combo_box_get_active_text(GTK_COMBO_BOX(m_pSamplingFrequency))));
		m_pHeader->setSubjectGender(
			l_sGender=="male"?OVTK_Value_Gender_Male:
			l_sGender=="female"?OVTK_Value_Gender_Female:
			l_sGender=="unknown"?OVTK_Value_Gender_Unknown:
			OVTK_Value_Gender_NotSpecified);
		for(uint32 i=0; i!=m_pHeader->getChannelCount(); i++)
		{
			if(m_vChannelName[i]!="")
			{
				m_pHeader->setChannelName(i, m_vChannelName[i].c_str());
			}
		}
	}

	gtk_widget_hide(m_pDialog);

	g_object_unref(m_pGladeConfigureInterface);
	g_object_unref(m_pGladeConfigureChannelInterface);
	m_pGladeConfigureInterface=NULL;
	m_pGladeConfigureChannelInterface=NULL;

	m_vChannelName.clear();

	return true;
}

void CConfigurationGlade::buttonChangeChannelNamesCB(::GtkButton* pButton)
{
	uint32 i;
	::GtkTreeIter itElectrodeName, itChannelName;
	::GtkDialog* l_pDialog=GTK_DIALOG(glade_xml_get_widget(m_pGladeConfigureChannelInterface, "channel-names"));
	::GtkTreeView* l_pElectrodeNameTreeView=GTK_TREE_VIEW(m_pElectrodeNameTreeView);
	::GtkTreeView* l_pChannelNameTreeView=GTK_TREE_VIEW(m_pChannelNameTreeView);

	// Creates electrode name and channel name models

	m_pElectrodeNameListStore=gtk_list_store_new(1, G_TYPE_STRING);
	m_pChannelNameListStore=gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

	// Fills in electrode name model

	list<string> l_vElectrodeName;
	list<string>::iterator l;
	ifstream l_oFile(m_sElectrodeFileName.c_str());
	if(l_oFile.is_open())
	{
		while(!l_oFile.eof())
		{
			string l_sElectrodeName;
			l_oFile >> l_sElectrodeName;
			l_vElectrodeName.push_back(l_sElectrodeName);
		}
		l_oFile.close();

		for(l=l_vElectrodeName.begin(); l!=l_vElectrodeName.end(); l++)
		{
			gtk_list_store_append(m_pElectrodeNameListStore, &itElectrodeName);
			gtk_list_store_set(m_pElectrodeNameListStore, &itElectrodeName, 0, l->c_str(), -1);
		}
	}

	// Fills in channel name model

	uint32 l_ui32ChannelCount=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(m_pNumberOfChannels));
	for(i=0; i<l_ui32ChannelCount; i++)
	{
		char l_sChannelName[1024];
		sprintf(l_sChannelName, "%i", (int)i);
		gtk_list_store_append(m_pChannelNameListStore, &itChannelName);
		gtk_list_store_set(m_pChannelNameListStore, &itChannelName, 0, l_sChannelName, -1);
		gtk_list_store_set(m_pChannelNameListStore, &itChannelName, 1, m_vChannelName[i].c_str(), -1);
	}

	// Attachs model to views

	gtk_tree_view_set_model(l_pElectrodeNameTreeView, GTK_TREE_MODEL(m_pElectrodeNameListStore));
	gtk_tree_view_set_model(l_pChannelNameTreeView, GTK_TREE_MODEL(m_pChannelNameListStore));

	// Runs dialog !

	if(gtk_dialog_run(l_pDialog)==GTK_RESPONSE_APPLY)
	{
		int i=0;
		gchar* l_sChannelName=NULL;
		if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(m_pChannelNameListStore), &itChannelName))
		{
			do
			{
				gtk_tree_model_get(GTK_TREE_MODEL(m_pChannelNameListStore), &itChannelName, 1, &l_sChannelName, -1);
				m_vChannelName[i++]=l_sChannelName;
			}
			while(gtk_tree_model_iter_next(GTK_TREE_MODEL(m_pChannelNameListStore), &itChannelName));
		}
	}

	gtk_widget_hide(GTK_WIDGET(l_pDialog));
	g_object_unref(m_pChannelNameListStore);
	g_object_unref(m_pElectrodeNameListStore);
	m_pChannelNameListStore=NULL;
	m_pElectrodeNameListStore=NULL;
}

void CConfigurationGlade::buttonApplyChannelNameCB(::GtkButton* pButton)
{
	::GtkTreeIter itElectrodeName, itChannelName;
	::GtkTreeView* l_pElectrodeNameTreeView=GTK_TREE_VIEW(m_pElectrodeNameTreeView);
	::GtkTreeView* l_pChannelNameTreeView=GTK_TREE_VIEW(m_pChannelNameTreeView);

	::GtkTreeSelection* l_pChannelNameTreeViewSelection=gtk_tree_view_get_selection(l_pChannelNameTreeView);
	::GtkTreeSelection* l_pElectrodeNameTreeViewSelection=gtk_tree_view_get_selection(l_pElectrodeNameTreeView);

	if(gtk_tree_selection_get_selected(l_pChannelNameTreeViewSelection, NULL, &itChannelName))
	{
		if(gtk_tree_selection_get_selected(l_pElectrodeNameTreeViewSelection, NULL, &itElectrodeName))
		{
			gchar* l_sElectrodeName=NULL;
			gtk_tree_model_get(GTK_TREE_MODEL(m_pElectrodeNameListStore), &itElectrodeName, 0, &l_sElectrodeName, -1);
			gtk_list_store_set(m_pChannelNameListStore, &itChannelName, 1, l_sElectrodeName, -1);
		}
	}
}

void CConfigurationGlade::buttonRemoveChannelNameCB(::GtkButton* pButton)
{
	::GtkTreeIter itChannelName;
	::GtkTreeView* l_pChannelNameTreeView=GTK_TREE_VIEW(m_pChannelNameTreeView);

	::GtkTreeSelection* l_pChannelNameTreeViewSelection=gtk_tree_view_get_selection(l_pChannelNameTreeView);
	if(gtk_tree_selection_get_selected(l_pChannelNameTreeViewSelection, NULL, &itChannelName))
	{
		gtk_list_store_set(m_pChannelNameListStore, &itChannelName, 1, "", -1);
	}
}
