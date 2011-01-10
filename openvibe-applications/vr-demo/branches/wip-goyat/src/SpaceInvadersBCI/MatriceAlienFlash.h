#ifndef __OpenViBEApplication_MATRICEALIENFLASH_H__
#define __OpenViBEApplication_MATRICEALIENFLASH_H__

class MatriceAlienFlash : public PseudoMatrice
{
public : 
	MatriceAlienFlash(SceneManager *mSM, String nom)
	{
		mSceneMgr=mSM;
		//mWalkSpeed=50;
		mDirection=Vector3::ZERO;
		mDestination=Vector3::ZERO; 

		//initialisation du vecteur de position absolue de la matrice
		Vector3 PositionAbsolue=getPositionAbsolue();
			
		SceneNode *nodeAlienFlash=mSceneMgr->getRootSceneNode()->createChildSceneNode("AlienFlashNode",PositionAbsolue);

		//Cr�ation de la pseudo matrice en nodes
		for (int i=0;i<Nalien;i++)
		{    	
			String nomNode=PseudoMatrice::createNomNode(i,nom);
			SceneNode *node = nodeAlienFlash->createChildSceneNode(nomNode,Vector3(Ogre::Real(i*EcartCaseX), 0, 0));
			for (int j=0;j<Malien;j++)
			{    	
				String nomVaisseauNode=createNomCaseNode(i,j,nom);
				tab[i][j]=uneCase(nomVaisseauNode,node,i,j);
			}
		}

		//Remplissage de la walkList
		createWalkList();
		
		//remplissage de l'�tat de flash et de cible
		CibleTarget=std::pair<int,int>(-1,-1);
		//
		for(int i=0;i<Nalien;i++)//r�apparition de tous les aliens et suppression des flashs
		  {
			for(int j=0;j<Malien;j++)
			  {
				m_tabState[i][j]=1; //0=undetermined ; 1=base ; 2=cible
			  }
		  }
	}
	
	//remplissage d'une case de la matrice
	Entity* uneCase(string nomNode,SceneNode* node,int i,int j)
	{
				Entity *ent = mSceneMgr->createEntity(nomNode,"cube.mesh");
				SceneNode *sousNode =node->createChildSceneNode(nomNode,Vector3(0,Ogre::Real(-j*EcartCaseY), 0));
				sousNode->showBoundingBox(false);
				sousNode->attachObject(ent);
#if DIAGONALE
				if ((i+j)%3==0)
				{
					ent->setMaterialName("Spaceinvader/Alienbis_1_anim");
					ent->setMaterialName("Spaceinvader/Aliengris_1_anim");
				}
				if ((i+j)%3==1)
				{
					ent->setMaterialName("Spaceinvader/Alienbis_2_anim");
					ent->setMaterialName("Spaceinvader/Aliengris_2_anim");
				}
				if ((i+j)%3==2)
				{
					ent->setMaterialName("Spaceinvader/Alienbis_3_anim");
					ent->setMaterialName("Spaceinvader/Aliengris_3_anim");
				}
#else
				if (j%3==0)
				{
					ent->setMaterialName("Spaceinvader/Alienbis_1_anim");
					ent->setMaterialName("Spaceinvader/Aliengris_1_anim");
				}
				if (j%3==1)
				{
					ent->setMaterialName("Spaceinvader/Alienbis_2_anim");
					ent->setMaterialName("Spaceinvader/Aliengris_2_anim");
				}
				if (j%3==2)
				{
					ent->setMaterialName("Spaceinvader/Alienbis_3_anim");
					ent->setMaterialName("Spaceinvader/Aliengris_3_anim");
				}
#endif
				return ent;
	}

	void reinitialisation(void)
	{
		Vector3 PositionAbsolue=getPositionAbsolue();
		Node *nodeAlienFlash=mSceneMgr->getRootSceneNode()->getChild("AlienFlashNode");
		nodeAlienFlash->setPosition(PositionAbsolue);//remise � la position de d�part
		createWalkList();//re-remplissage de la Walklist
		for(int i=0;i<Nalien;i++)//r�apparition de tous les aliens et suppression des flashs
		{
			for(int j=0;j<Malien;j++)
			{faireApparaitreCase(i,j);}
			deflasherColonne(i);
		}
		
	}
	
	Vector3 getCoordonneesCase(int i, int j)
	{
		Vector3 Coord;
		Coord=tab[i][j]->getParentSceneNode()->getPosition();
		Coord=Coord+tab[i][j]->getParentSceneNode()->getParentSceneNode()->getPosition();
		Coord=Coord+tab[i][j]->getParentSceneNode()->getParentSceneNode()->getParentSceneNode()->getPosition();
		return Coord;
	}
	
	void exploseCase(int i, int j)
	{
		tab[i][j]->setMaterialName("Spaceinvader/Explosion");
	}
	
	void faireDisparaitreCase(int i, int j)
	{
		tab[i][j]->setVisible(false);
		m_tabState[i][j]=0;
		tab[i][j]->setMaterialName("Spaceinvader/Vide");
	}
	void faireApparaitreCase(int i, int j)
	{
		tab[i][j]->setVisible(true);
		m_tabState[i][j]=1;
		deFlasher(i,j);
	}
	void changerVisibiliteCase(int i, int j)
	{
		tab[i][j]->setVisible(!tab[i][j]->isVisible());
	}
	void faireDisparaitreColonne(int i)
	{
		for (int j=0;j<Malien;j++)
		{
			tab[i][j]->setVisible(false);
			m_tabState[i][j]=0;
		}
	}
	void faireDisparaitreLigne(int j)
	{
		for (int i=0;i<Nalien;i++)
		{
			tab[i][j]->setVisible(false);
			m_tabState[i][j]=0;
		}
	}
	void faireApparaitreColonne(int i)
	{
		for (int j=0;j<Malien;j++)
		{
			tab[i][j]->setVisible(true);
			m_tabState[i][j]=1;
		}
	}
	void faireApparaitreLigne(int j)
	{
		for (int i=0;i<Nalien;i++)
		{
			tab[i][j]->setVisible(true);
			m_tabState[i][j]=1;
		}
	}

	
	void deFlasher(int i, int j)
	{
	#if DIAGONALE
			if ((i+j)%3==0)
			  {
				if(m_tabState[i][j]==2)
					tab[i][j]->setMaterialName("Spaceinvader/Alien_1_Target_anim");
				else if(m_tabState[i][j]==1)
					tab[i][j]->setMaterialName("Spaceinvader/Aliengris_1_anim");
			  }
			if ((i+j)%3==1)
			  {
				if(m_tabState[i][j]==2)
					tab[i][j]->setMaterialName("Spaceinvader/Alien_2_Target_anim");
				else if(m_tabState[i][j]==1)
					tab[i][j]->setMaterialName("Spaceinvader/Aliengris_2_anim");
			  }
			if ((i+j)%3==2)
			  {
				if(m_tabState[i][j]==2)
					tab[i][j]->setMaterialName("Spaceinvader/Alien_3_Target_anim");
				else if(m_tabState[i][j]==1)
					tab[i][j]->setMaterialName("Spaceinvader/Aliengris_3_anim");
			  }
	#else
			if (j%3==0)
			  {
				if(m_tabState[i][j]==2)
					tab[i][j]->setMaterialName("Spaceinvader/Alien_1_Target_anim");
				else if(m_tabState[i][j]==1)
					tab[i][j]->setMaterialName("Spaceinvader/Aliengris_1_anim");
			  }
			if (j%3==1)
			  {
				if(m_tabState[i][j]==2)
					tab[i][j]->setMaterialName("Spaceinvader/Alien_2_Target_anim");
				else if(m_tabState[i][j]==1)
					tab[i][j]->setMaterialName("Spaceinvader/Aliengris_2_anim");
			  }
			if (j%3==2)
			  {
				if(m_tabState[i][j]==2)
					tab[i][j]->setMaterialName("Spaceinvader/Alien_3_Target_anim");
				else if(m_tabState[i][j]==1)
					tab[i][j]->setMaterialName("Spaceinvader/Aliengris_3_anim");
			  }
	#endif
	}
	
	void deflasherColonne(int i)
	{
		for (int j=0;j<Malien;j++)
		{
			deFlasher(i,j);
		}
	}
	
	void deflasherLigne(int j)
	{
		for (int i=0;i<Nalien;i++)
		{
			deFlasher(i,j);
		}
	}
	
	void flasher(int i, int j)
	{
	#if DIAGONALE
			if ((i+j)%3==0)
			  {
				if(m_tabState[i][j]==2)
					tab[i][j]->setMaterialName("Spaceinvader/Alien_1_TargetFlash_anim");
				else if(m_tabState[i][j]==1)
					tab[i][j]->setMaterialName("Spaceinvader/Alienbis_1_anim");
			  }
			if ((i+j)%3==1)
			  {
				if(m_tabState[i][j]==2)
					tab[i][j]->setMaterialName("Spaceinvader/Alien_2_TargetFlash_anim");
				else if(m_tabState[i][j]==1)
					tab[i][j]->setMaterialName("Spaceinvader/Alienbis_2_anim");
			  }
			if ((i+j)%3==2)
			  {
				if(m_tabState[i][j]==2)
					tab[i][j]->setMaterialName("Spaceinvader/Alien_3_TargetFlash_anim");
				else if(m_tabState[i][j]==1)
					tab[i][j]->setMaterialName("Spaceinvader/Alienbis_3_anim");
			  }
	#else
			if (j%3==0)
			  {
				if(m_tabState[i][j]==2)
					tab[i][j]->setMaterialName("Spaceinvader/Alien_1_TargetFlash_anim");
				else if(m_tabState[i][j]==1)
					tab[i][j]->setMaterialName("Spaceinvader/Alienbis_1_anim");
			  }
			if (j%3==1)
			  {
				if(m_tabState[i][j]==2)
					tab[i][j]->setMaterialName("Spaceinvader/Alien_2_TargetFlash_anim");
				else if(m_tabState[i][j]==1)
					tab[i][j]->setMaterialName("Spaceinvader/Alienbis_2_anim");
			  }
			if (j%3==2)
			  {
				if(m_tabState[i][j]==2)
					tab[i][j]->setMaterialName("Spaceinvader/Alien_3_TargetFlash_anim");
				else if(m_tabState[i][j]==1)
					tab[i][j]->setMaterialName("Spaceinvader/Alienbis_3_anim");
			  }
	#endif
	}
	
	void flasherColonne(int i)
	{
		for (int j=0;j<Malien;j++)
		{
			flasher(i,j);
		}
	}
	
	void flasherLigne(int j)
	{
		for (int i=0;i<Nalien;i++)
		{
			flasher(i,j);
		}
	}

	bool setTarget(std::pair<int,int> pr)
	{
		if(pr.first>=Nalien || pr.first<0 || pr.second>=Malien || pr.second<0) 
		  {return false;}
		
		m_tabState[pr.first][pr.second]=2;
		//
		#if DIAGONALE
			if ((pr.first+pr.second)%3==0)
				tab[pr.first][pr.second]->setMaterialName("Spaceinvader/Alien_1_Target_anim");
			if ((pr.first+pr.second)%3==1)
				tab[pr.first][pr.second]->setMaterialName("Spaceinvader/Alien_2_Target_anim");
			if ((pr.first+pr.second)%3==2)
				tab[pr.first][pr.second]->setMaterialName("Spaceinvader/Alien_3_Target_anim");
		#else
			if (pr.second%3==0)
				tab[pr.first][pr.second]->setMaterialName("Spaceinvader/Alien_1_Target_anim");
			if (pr.second%3==1)
				tab[pr.first][pr.second]->setMaterialName("Spaceinvader/Alien_2_Target_anim");
			if (pr.second%3==2)
				tab[pr.first][pr.second]->setMaterialName("Spaceinvader/Alien_3_Target_anim");
		#endif
		return true;
	}
	
	bool changeTarget(std::pair<int,int> pr)
	{
		if(CibleTarget.first<Nalien || CibleTarget.first>=0 || CibleTarget.second<Malien || CibleTarget.second>=0)  
		  {
			m_tabState[CibleTarget.first][CibleTarget.second]=1;
			deFlasher(CibleTarget.first,CibleTarget.second);
		  }
		CibleTarget=pr;
		//
		return setTarget(pr);
	}
	
	Vector3 getEcartCase()
	{
		return Vector3(Ogre::Real(EcartCaseX),Ogre::Real(EcartCaseY),0);
	}
	
	Vector3 getPositionAbsolue()
	{
		return Vector3(Ogre::Real(PositionAbsolueX),Ogre::Real(PositionAbsolueY),Ogre::Real(PositionAbsolueZ));
	}
	
	bool ligneIsVisible(int i) //retourne true s'il reste un alien visible sur la ligne
	{
		bool result=false;
		for (int j=0;j<Nalien;j++)
		{
			result = result || tab[i][j]->isVisible();
		}
		return result;
	}
	
	bool colonneIsVisible(int j) //retourne true s'il reste un alien visible sur la colonne
	{
		bool result=false;
		for (int i=0;i<Malien;i++)
		{
			result = result || tab[i][j]->isVisible();
		}
		return result;
	}
		
	bool alienIsVisible(int i,int j)
	{
		return tab[i][j]->isVisible();
	}
	
	SceneNode * alienPositionne(Vector3 Coord, Vector3 Marge)
	{
		//std::cout<<"alienPositionne"<<std::endl;
		//cr�ation d'un double it�rateur sur la liste d'aliens
		//et selection d'un alien visible et dans la boite d�sir�e
		Ogre::SceneNode::ChildNodeIterator iterateurNode = tab[0][0]->getParentSceneNode()->getParentSceneNode()->getParentSceneNode()->getChildIterator();
		SceneNode * Resultat =NULL;

		Vector3 CoordRoot=tab[0][0]->getParentSceneNode()->getParentSceneNode()->getParentSceneNode()->getPosition();

		//std::cout<<"start While..."<<std::endl;
		while(iterateurNode.hasMoreElements()&&(Resultat==NULL))
		{
			Node *NodeCourant=iterateurNode.getNext();
			Ogre::SceneNode::ChildNodeIterator iterateurSousNode = NodeCourant->getChildIterator();
			Vector3 CoordNode=NodeCourant->getPosition()+CoordRoot;

			while(iterateurSousNode.hasMoreElements()&&(Resultat==NULL))
			{
				SceneNode * SousNodeCourant=(SceneNode *)iterateurSousNode.getNext();
				Vector3 CoordSousNode=SousNodeCourant->getPosition()+CoordNode;
				Ogre::SceneNode::ObjectIterator iterateurAlien = SousNodeCourant->getAttachedObjectIterator();

				while(iterateurAlien.hasMoreElements()&&(Resultat==NULL))
				{
					Entity * AlienCourant=(Entity*)iterateurAlien.getNext();
					if((AlienCourant->isVisible()) && (abs(CoordSousNode.x-Coord.x)<Marge.x) && (abs(CoordSousNode.y-Coord.y)<Marge.y))
					{Resultat=SousNodeCourant;}
				}
			}
		}
		return Resultat;
	}

protected :
	static const int EcartCaseX=100;//=150
	static const int EcartCaseY=100;//=150
#if MOBILE
	static const int PositionAbsolueX=-300;
	static const int PositionAbsolueY=700;
	static const int PositionAbsolueZ=0;
#else
	static const int PositionAbsolueX=0;//=-100
	static const int PositionAbsolueY=700;
	static const int PositionAbsolueZ=0;
#endif

	int m_tabState[Nalien][Malien];
	std::pair<int,int> CibleTarget; //-1 = non d�clar�
};

#endif