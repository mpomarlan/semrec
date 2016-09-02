/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2013, Institute for Artificial Intelligence,
 *  Universität Bremen.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the Institute for Artificial Intelligence,
 *     Universität Bremen, nor the names of its contributors may be
 *     used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

/** \author Jan Winkler */


#include <semrec/plugins/owlexporter/CExporterOwl.h>


namespace semrec {
  CExporterOwl::CExporterOwl() {
    m_strPropertyNamespace = "";
    m_strDefaultAnnotation = "";
    
    this->setMessagePrefixLabel("owl-exporter-aux");
  }
  
  CExporterOwl::~CExporterOwl() {
  }
  
  void CExporterOwl::setMetaData(std::map<std::string, MappedMetaData> mapMetaData) {
    m_mapMetaData = mapMetaData;
    
    // NOTE(winkler): This is a hack and needs to be resolved later.
    if(mapMetaData.find("robot") != mapMetaData.end()) {
      OwlIndividual::addStaticProperty("robot", mapMetaData["robot"].strValue);
    }
  }
  
  bool CExporterOwl::loadSemanticsDescriptorFile(std::string strFilepath) {
    if(this->fileExists(strFilepath)) {
      libconfig::Config cfgConfig;
      
      try {
	cfgConfig.readFile(strFilepath.c_str());
	
	if(cfgConfig.exists("condition-mappings")) {
	  libconfig::Setting &sConditionMappings = cfgConfig.lookup("condition-mappings");
	  
	  if(sConditionMappings.exists("mappings")) {
	    libconfig::Setting &sMappings = sConditionMappings["mappings"];
	    
	    for(int nI = 0; nI < sMappings.getLength(); nI++) {
	      std::string strTo;
	      
	      if(sMappings[nI].lookupValue("to", strTo)) {
		if(sMappings[nI].exists("from")) {
		  libconfig::Setting &sFrom = sMappings[nI]["from"];
		  
		  for(int nJ = 0; nJ < sFrom.getLength(); nJ++) {
		    std::string strFrom = sFrom[nJ];
		    
		    m_lstFailureMapping.push_back(std::make_pair(strFrom, strTo));
		  }
		}
	      } else {
		this->warn("Condition mapping without 'to' field. Ignoring.");
	      }
	    }
	  }
	  
	  if(sConditionMappings.exists("default-condition-mapping")) {
	    sConditionMappings.lookupValue("default-condition-mapping", m_strDefaultConditionMapping);
	  } else {
	    this->warn("No default condition mapping set. Assuming empty string.");
	    m_strDefaultConditionMapping = "";
	  }
	}
	
	m_lstAnnotationPurposeMapping.clear();
	
	if(cfgConfig.exists("structure")) {
	  libconfig::Setting &sStructure = cfgConfig.lookup("structure");
	  
	  m_strPropertyNamespace = "";
	  if(sStructure.exists("property-namespace")) {
	    sStructure.lookupValue("property-namespace", m_strPropertyNamespace);
	  }
	  
	  if(m_strPropertyNamespace == "") {
	    this->warn("You didn't specify the 'structure/property-namespace' parameter on the semantics descriptor file. Your OWL classes will have no namespace prepended. Is this intended?");
	  }
	  
	  m_strDefaultDesignatorClass = "";
	  if(sStructure.exists("default-designator-class")) {
	    sStructure.lookupValue("default-designator-class", m_strDefaultDesignatorClass);
	  }
	  
	  if(m_strDefaultDesignatorClass == "") {
	    this->warn("You didn't specify the 'structure/default-designator-class' parameter in the semantics descriptor file. Your default designators will have no OWL class, resulting in an invalid OWL file. Is this intended?");
	  }
	  
	  m_strDefaultAnnotation = "";
	  if(sStructure.exists("default-annotation-purpose")) {
	    sStructure.lookupValue("default-annotation-purpose", m_strDefaultAnnotation);
	  }
	  
	  if(m_strDefaultAnnotation == "") {
	    this->warn("You didn't specify the 'structure/default-annotation-purpose' parameter on the semantics descriptor file. Your designator attachments without a defined annotation will be empty and produce a faulty OWL file. Is this intended?");
	  }
	  
	  if(sStructure.exists("annotation-purposes")) {
	    libconfig::Setting &sPurposes = sStructure["annotation-purposes"];
	    
	    for(int nI = 0; nI < sPurposes.getLength(); nI++) {
	      libconfig::Setting &sPurpose = sPurposes[nI];
	      
	      std::string strFrom;
	      std::string strTo;
	      
	      sPurpose.lookupValue("from", strFrom);
	      sPurpose.lookupValue("to", strTo);
	      
	      if(strFrom != "" && strTo != "") {
		m_lstAnnotationPurposeMapping.push_back(std::make_pair(strFrom, strTo));
	      } else {
		this->warn("Invalid annotation purpose mapping: '" + strFrom + "' -> '" + strTo + "'. Discarding.");
	      }
	    }
	  }
	}
	
	return true;
      } catch(libconfig::ParseException e) {
        std::stringstream sts;
        sts << e.getLine();
	
        this->fail("Error while parsing semantics descriptor file '" + strFilepath + "': " + e.getError() + ", on line " + sts.str());
      } catch(...) {
	this->fail("Undefined error while parsing semantics descriptor file '" + strFilepath + "'");
      }
    } else {
      this->fail("Semantics descriptor file not found: '" + strFilepath + "'.");
    }
    
    return false;
  }
  
  void CExporterOwl::prepareEntities(std::string strNamespaceID, std::string strNamespace) {
    m_lstEntities.clear();
  
    this->addEntity("owl", "http://www.w3.org/2002/07/owl#");
    this->addEntity("xsd", "http://www.w3.org/2001/XMLSchema#");
    this->addEntity("knowrob", "http://knowrob.org/kb/knowrob.owl#");
    this->addEntity("rdfs", "http://www.w3.org/2000/01/rdf-schema#");
    this->addEntity("rdf", "http://www.w3.org/1999/02/22-rdf-syntax-ns#");
    this->addEntity(strNamespaceID, strNamespace + "#");
    
    for(std::pair<std::string, std::string> prEntity : m_mapRegisteredOWLNamespaces) {
      if(prEntity.second != "") {
	this->addEntity(prEntity.first, prEntity.second);
      }
    }
  }
  
  void CExporterOwl::setRegisteredOWLNamespaces(std::map<std::string, std::string> mapRegisteredOWLNamespaces) {
    m_mapRegisteredOWLNamespaces = mapRegisteredOWLNamespaces;
  }
  
  void CExporterOwl::addEntity(std::string strNickname, std::string strNamespace) {
    m_lstEntities.push_back(std::make_pair(strNickname, strNamespace));
  }

  std::string CExporterOwl::generateDocTypeBlock() {
    std::string strDot = "<!DOCTYPE rdf:RDF [\n";
    
    for(std::pair<std::string, std::string> prEntity : m_lstEntities) {
      strDot += "    <!ENTITY " + prEntity.first + " \"" + prEntity.second + "\">\n";
    }
  
    strDot += "]>\n\n";
    
    return strDot;
  }

  std::string CExporterOwl::generateXMLNSBlock(std::string strNamespace) {
    std::string strDot = "<rdf:RDF xmlns=\"" + strNamespace + "#\"\n";
    strDot += "     xml:base=\"" + strNamespace + "\"\n";
    
    for(std::list< std::pair<std::string, std::string> >::iterator itPair = m_lstEntities.begin();
	itPair != m_lstEntities.end();
	itPair++) {
      std::pair<std::string, std::string> prEntity = *itPair;
      
      if(itPair != m_lstEntities.begin()) {
	strDot += "\n";
      }

      strDot += "     xmlns:" + prEntity.first + "=\"" + prEntity.second + "\"";
    }

    strDot += ">\n\n";
    return strDot;
  }
  
  std::string CExporterOwl::generateOwlImports(std::string strNamespace) {
    std::string strDot = "";
    std::string strImportNamespace = "package://knowrob_common/owl/knowrob.owl";
    
    strDot += "    <owl:Ontology rdf:about=\"" + strNamespace + "\">\n";
    strDot += "        <owl:imports rdf:resource=\"" + strImportNamespace + "\"/>\n";
    strDot += "    </owl:Ontology>\n\n";
    
    return strDot;
  }

  std::string CExporterOwl::generatePropertyDefinitions() {
    std::string strDot = "    <!-- Property Definitions -->\n\n";
    
    std::list<std::string> lstProperties = OwlIndividual::issuedProperties();
    for(std::string strProperty : lstProperties) {
      strDot += "    <owl:ObjectProperty rdf:about=\"" + strProperty + "\"/>\n\n";
    }
    
    return strDot;
  }
  
  std::list<std::string> CExporterOwl::gatherClassesForNodes(std::list<Node*> lstNodes) {
    std::list<std::string> lstClasses;
    
    for(Node* ndCurrent : lstNodes) {
      if(ndCurrent) {
	std::list<std::string> lstClassesSubnodes = this->gatherClassesForNodes(ndCurrent->subnodes());
	
	if(ndCurrent->metaInformation()->stringValue("class") != "") {
	  lstClassesSubnodes.push_back(ndCurrent->metaInformation()->stringValue("classnamespace") + ndCurrent->metaInformation()->stringValue("class"));
	} else {
	  lstClassesSubnodes.push_back(this->owlClassForNode(ndCurrent));
	}
	
	for(std::string strClassSubnode : lstClassesSubnodes) {
	  bool bExists = false;
	  
	  for(std::string strClassNode : lstClasses) {
	    if(strClassSubnode == strClassNode) {
	      bExists = true;
	      break;
	    }
	  }
	  
	  if(!bExists) {
	    lstClasses.push_back(strClassSubnode);
	  }
	}
      } else {
	this->fail("Class for invalid node requested!");
      }
    }
    
    return lstClasses;
  }
  
  std::list<std::string> CExporterOwl::gatherTimepointsForNodes(std::list<Node*> lstNodes, std::list<Node*> lstTrace) {
    std::list<std::string> lstTimepoints;
    
    for(Node* ndCurrent : lstNodes) {
      if(ndCurrent) {
	if(std::find(lstTrace.begin(), lstTrace.end(), ndCurrent) == lstTrace.end()) {
	  // Gather node timepoints
	  std::list<Node*> lstSubTrace = lstTrace;
	  lstSubTrace.push_back(ndCurrent);
	  
	  std::list<std::string> lstTimepointsSubnodes = this->gatherTimepointsForNodes(ndCurrent->subnodes(), lstSubTrace);
	  lstTimepointsSubnodes.push_back(ndCurrent->metaInformation()->stringValue("time-start"));
	  lstTimepointsSubnodes.push_back(ndCurrent->metaInformation()->stringValue("time-end"));
	  lstTimepointsSubnodes.push_back(ndCurrent->metaInformation()->stringValue("time-start-system"));
	  
	  // Gather failure timepoints
	  KeyValuePair* ckvpFailures = ndCurrent->metaInformation()->childForKey("failures");
	  
	  if(ckvpFailures) {
	    std::list<KeyValuePair*> lstFailures = ckvpFailures->children();
	    
	    unsigned int unIndex = 0;
	    for(KeyValuePair* ckvpFailure : lstFailures) {
	      lstTimepointsSubnodes.push_back(ckvpFailure->stringValue("time-fail"));
	    }
	  }
	  
	  // Gather image timepoints
	  KeyValuePair* ckvpImages = ndCurrent->metaInformation()->childForKey("images");
	  
	  if(ckvpImages) {
	    std::list<KeyValuePair*> lstImages = ckvpImages->children();
	  
	    unsigned int unIndex = 0;
	    for(KeyValuePair* ckvpImage : lstImages) {
	      lstTimepointsSubnodes.push_back(ckvpImage->stringValue("time-capture"));
	    }
	  }
	
	  // Gather designator equation timepoints
	  for(std::pair<std::string, std::string> prPair : m_lstDesignatorEquationTimes) {
	    lstTimepointsSubnodes.push_back(prPair.second);
	  }
	
	  // Gather designator creation timepoints
	  for(std::pair<std::string, KeyValuePair*> prDesig : m_mapDesignators) {
	    if(prDesig.second) {
	      if(prDesig.second->childForKey("description")) {
		lstTimepointsSubnodes.push_back(prDesig.second->childForKey("description")->stringValue("_time_created"));
	      }
	    }
	  }
	  
	  std::copy(lstTimepointsSubnodes.begin(), lstTimepointsSubnodes.end(),
		    std::back_insert_iterator<std::list<std::string> >(lstTimepoints));
	}
      } else {
	this->fail("Timepoints for invalid node requested!");
      }
    }
    
    return lstTimepoints;
  }
  
  std::string CExporterOwl::generateClassDefinitions() {
    std::string strDot = "    <!-- Class Definitions -->\n\n";
    
    std::list<std::string> lstClasses = OwlIndividual::issuedTypes();
    
    for(std::string strClass : lstClasses) {
      strDot += "    <owl:Class rdf:about=\"" + strClass + "\"/>\n\n";
    }
    
    return strDot;
  }
  
  std::string CExporterOwl::nodeIDPrefix(Node* ndInQuestion, std::string strProposition) {
    std::string strPrefix = CExporter::nodeIDPrefix(ndInQuestion, strProposition);
    std::string strOwlClass = this->owlClassForNode(ndInQuestion, true);
    
    if(strPrefix == "") {
      strPrefix = strOwlClass + "_";
    } else if(strPrefix == strProposition) {
      if(strOwlClass != "") {
	strPrefix = strOwlClass + "_";
      }
    }
    
    return strPrefix;
  }
  
  std::string CExporterOwl::generateEventIndividualsForNodes(std::list<Node*> lstNodes, std::string strNamespace) {
    std::string strDot = "";
    
    m_mapDesignators.clear();
    
    Node* ndLastDisplayed = NULL;
    for(std::list<Node*>::iterator itNode = lstNodes.begin();
	itNode != lstNodes.end();
	itNode++) {
      Node* ndCurrent = *itNode;
      
      if(ndCurrent) {
	if(this->nodeDisplayable(ndCurrent)) {
	  std::string strOwlClass;
	  
	  if(ndCurrent->metaInformation()->stringValue("class") != "") {
	    strOwlClass = ndCurrent->metaInformation()->stringValue("classnamespace") + ndCurrent->metaInformation()->stringValue("class");
	  } else {
	    strOwlClass = this->owlClassForNode(ndCurrent);
	  }
	  
	  // NOTE: The position of this line causes that newer nodes
	  // are always further up in the log. If this is not
	  // intended, move it below the generation code for the
	  // current node.
	  strDot += this->generateEventIndividualsForNodes(ndCurrent->subnodes(), strNamespace);
	  
	  // NOTE: Here, the generation code the the current node
	  // begins.
	  OwlIndividual oiIndividual;
	  oiIndividual.setID("&" + strNamespace + ";" + ndCurrent->uniqueID());
	  oiIndividual.setType(strOwlClass);
	  
	  oiIndividual.addDataProperty("knowrob:taskContext", "&xsd;string", ndCurrent->title());
	  oiIndividual.addDataProperty("knowrob:taskSuccess", "&xsd;boolean", (ndCurrent->success() ? std::string("true") : std::string("false")));
	  oiIndividual.addResourceProperty("knowrob:startTime", "&" + strNamespace + ";timepoint_" + ndCurrent->metaInformation()->stringValue("time-start"));
	  oiIndividual.addResourceProperty("knowrob:endTime", "&" + strNamespace + ";timepoint_" + ndCurrent->metaInformation()->stringValue("time-end"));
	  oiIndividual.addResourceProperty("knowrob:startTimeSystem", "&" + strNamespace + ";timepoint_" + ndCurrent->metaInformation()->stringValue("time-start-system"));
	  
	  if(ndCurrent->title() == "GOAL-ACHIEVE") {
	    std::list<KeyValuePair*> lstDescription = ndCurrent->description();
	    std::string strPattern = "";
	    
	    for(KeyValuePair* ckvpNow : lstDescription) {
	      if(ckvpNow->key() == "PATTERN") {
		strPattern = ckvpNow->stringValue();
		break;
	      }
	    }
	    
	    if(strPattern != "") {
	      oiIndividual.addDataProperty("knowrob:goalContext", "&xsd;string", strPattern);
	    }
	  }

	  if(ndCurrent->title() == "HUMAN-INTRUSION") {
            this->info("writing out human intrusion.");
	    std::list<KeyValuePair*> lstDescription = ndCurrent->description();
	    std::string strBodypart = "";
	    
	    for(KeyValuePair* ckvpNow : lstDescription) {
	      if(ckvpNow->key() == "_BODYPART") {
                this->info("found bodypart.");
		oiIndividual.addDataProperty("knowrob:bodyPartsUsed", "&xsd;string", ckvpNow->stringValue());
		continue;
	      }
	      if(ckvpNow->key() == "_HUMANDESIG") {
                this->info("found human desig.");
		oiIndividual.addResourceProperty("knowrob:designator", 
                    "&" + strNamespace + ";" + ckvpNow->stringValue());
		continue;
	      }
	    }
	  }

          // NOTE FROM GEORG: This is a feature that I introduced specifically for the 
          // final review of SAPHARI. I think it needs to be improved, but time has not
          // permitted a refactoring. Sorry!
          // 
          // The intention: We had a semantic map with individuals of objects, i.e.
          // baskets full of sorted objects, that we would reference to assert goals for
          // the robot at runtime. By generating an Individual of type &saphari;SaphariTaskDescription
          // which points to one of these individuals in the semantic mal we would
          // select the task for the robot. The following code block points to such an
          // basket individual for the task description event.
          //
          // What could be improved: Have formal representations for goal states in
          // Knowrob, and then a dedicated channel in semrec+knowrob_cram for asserting
          // such goals.
          if(ndCurrent->title() == "SEMANTIC-MAP-TARGET-OBJECT"){
            this->info("linking to target object in semantic map");
            std::list<KeyValuePair*> lstDescription = ndCurrent->description();

            for(KeyValuePair* ckvpNow : lstDescription) {
              if(ckvpNow->key() == "_TARGETOBJECT") {
                this->info("found target object");
                oiIndividual.addResourceProperty("knowrob:objectActedOn", ckvpNow->stringValue());
                continue;
              }
            }
          }

  
	  std::list<Node*> lstSubnodes = ndCurrent->subnodes();
	  for(Node* ndSubnode : lstSubnodes) {
	    if(this->nodeDisplayable(ndSubnode)) {
	      oiIndividual.addResourceProperty("knowrob:subAction", "&" + strNamespace + ";" + ndSubnode->uniqueID());
	    }
	  }
	  
	  if(ndLastDisplayed) {
	    oiIndividual.addResourceProperty("knowrob:previousAction", "&" + strNamespace + ";" + ndLastDisplayed->uniqueID());
	  }
	  
	  std::list<Node*>::iterator itPostEvent = itNode;
	  itPostEvent++;
	  while(itPostEvent != lstNodes.end()) {
	    if(this->nodeDisplayable(*itPostEvent)) {
	      oiIndividual.addResourceProperty("knowrob:nextAction", "&" + strNamespace + ";" + (*itPostEvent)->uniqueID());
	      break;
	    }
	    
	    itPostEvent++;
	  }
	  
	  // Object references here.
	  KeyValuePair* ckvpObjects = ndCurrent->metaInformation()->childForKey("objects");
	  
	  if(ckvpObjects) {
	    std::list<KeyValuePair*> lstObjects = ckvpObjects->children();
	    
	    unsigned int unIndex = 0;
	    for(KeyValuePair* ckvpObject : lstObjects) {
	      std::string strDefClass = ckvpObject->stringValue("_class");
	      std::string strDefClassNamespace = ckvpObject->stringValue("_classnamespace");
	      std::string strDefProperty = ckvpObject->stringValue("_property");
	      
	      if(strDefClass == "") {
		strDefClass = "object";
	      }
	      
	      if(strDefProperty == "") {
		if(strOwlClass == "&knowrob;UIMAPerception"){ //VisualPerception") {
		  strDefProperty = "knowrob:perceptionResult"; //detectedObject";
		} else {
		  strDefProperty = "knowrob:objectActedOn";
		}
	      }
	      
	      if(strDefClassNamespace == "") {
		strDefClassNamespace = "&" + strNamespace + ";";
	      }
	      
	      std::string strObjectID = strDefClass + "_" + ckvpObject->stringValue("__id");
	      oiIndividual.addResourceProperty(strDefProperty, strDefClassNamespace + strObjectID);
	    }
	  }

	  // Human references here.
	  KeyValuePair* ckvpHumans = ndCurrent->metaInformation()->childForKey("humans");
	  
	  if(ckvpHumans) {
	    std::list<KeyValuePair*> lstHumans = ckvpHumans->children();
	    
	    unsigned int unIndex = 0;
	    for(KeyValuePair* ckvpHuman : lstHumans) {
	      std::string strDefClass = ckvpHuman->stringValue("_class");
	      std::string strDefClassNamespace = ckvpHuman->stringValue("_classnamespace");
	      std::string strDefProperty = ckvpHuman->stringValue("_property");
	      
	      if(strDefClass == "") {
		strDefClass = "human";
	      }
	      
	      if(strDefProperty == "") {
	        strDefProperty = "knowrob:detectedPerson";
	      }
	      
	      if(strDefClassNamespace == "") {
		strDefClassNamespace = "&" + strNamespace + ";";
	      }
	      
	      std::string strObjectID = strDefClass + "_" + ckvpHuman->stringValue("__id");
	      oiIndividual.addResourceProperty(strDefProperty, strDefClassNamespace + strObjectID);
	    }
	  }
  
	  // Image references here.
	  KeyValuePair* ckvpImages = ndCurrent->metaInformation()->childForKey("images");
	  
	  if(ckvpImages) {
	    std::list<KeyValuePair*> lstImages = ckvpImages->children();
	    
	    unsigned int unIndex = 0;
	    for(KeyValuePair* ckvpImage : lstImages) {
	      std::stringstream sts;
	      sts << ndCurrent->uniqueID() << "_image_" << unIndex;
	      
	      oiIndividual.addResourceProperty("knowrob:capturedImage", "&" + strNamespace + ";" + sts.str());
	    }
	  }
	  
	  // Failure references here.
	  KeyValuePair* ckvpFailures = ndCurrent->metaInformation()->childForKey("failures");
	  
	  if(ckvpFailures) {
	    std::list<KeyValuePair*> lstFailures = ckvpFailures->children();
	    
	    unsigned int unIndex = 0;
	    for(KeyValuePair* ckvpFailure : lstFailures) {
	      std::stringstream sts;
	      sts << ndCurrent->uniqueID() << "_failure_" << unIndex;
	      oiIndividual.addResourceProperty("knowrob:eventFailure", "&" + strNamespace + ";" + sts.str());
	      m_nThrowAndCatchFailureCounter++;
	    }
	  }
	  
	  // Caught failure here.
	  KeyValuePair* ckvpCaughtFailures = ndCurrent->metaInformation()->childForKey("caught_failures");
	  
	  if(ckvpCaughtFailures) {
	    std::list<KeyValuePair*> lstCaughtFailures = ckvpCaughtFailures->children();
	    
	    unsigned int unIndex = 0;
	    for(KeyValuePair* ckvpCaughtFailure : lstCaughtFailures) {
	      Node* ndFailureEmitter = ndCurrent->emitterForCaughtFailure(ckvpCaughtFailure->stringValue("failure-id"), ckvpCaughtFailure->stringValue("emitter-id"), ckvpCaughtFailure->stringValue("time-catch"));
	      
	      if(ndFailureEmitter) {
		std::string strCaughtFailure = ndFailureEmitter->uniqueID() + "_" + ckvpCaughtFailure->stringValue("failure-id");
		m_nThrowAndCatchFailureCounter--;
		oiIndividual.addResourceProperty("knowrob:caughtFailure", "&" + strNamespace + ";" + strCaughtFailure);
	      } else {
		this->warn("No emitter for failure '" + ckvpCaughtFailure->stringValue("failure-id") + "'.");
	      }
	    }
	  }
	  
	  // Designator references here.
	  KeyValuePair* ckvpDesignators = ndCurrent->metaInformation()->childForKey("designators");
	  
	  if(ckvpDesignators) {
	    std::list<KeyValuePair*> lstDesignators = ckvpDesignators->children();
	    
	    unsigned int unIndex = 0;
	    for(KeyValuePair* ckvpDesignator : lstDesignators) {
	      std::string strAnnotation = ckvpDesignator->stringValue("annotation");
	      
	      if(this->resolveDesignatorAnnotationTagName(strAnnotation) == "speechActDetails") {
		KeyValuePair* kvpSpeechAct = ckvpDesignator->childForKey("description");
		
		std::string strSender = kvpSpeechAct->stringValue("sender");
		std::string strReceiver = kvpSpeechAct->stringValue("receiver");
		
		if(strSender == "PR2") {
		  oiIndividual.addResourceProperty("knowrob:sender", "http://knowrob.org/kb/PR2.owl#PR2");
		} else if(strSender == "Boxy") {
		  oiIndividual.addResourceProperty("knowrob:sender", "http://knowrob.org/kb/Boxy.owl#boxy_robot1");
		} else {
		  oiIndividual.addDataProperty("knowrob:sender", "&xsd;string", strSender);
		}
		
		if(strReceiver == "PR2") {
		  oiIndividual.addResourceProperty("knowrob:receiver", "http://knowrob.org/kb/PR2.owl#PR2");
		} else if(strReceiver == "Boxy") {
		  oiIndividual.addResourceProperty("knowrob:receiver", "http://knowrob.org/kb/Boxy.owl#boxy_robot1");
		} else {
		  oiIndividual.addDataProperty("knowrob:receiver", "&xsd;string", strReceiver);
		}
		
		oiIndividual.addDataProperty("knowrob:content", "&xsd;string", kvpSpeechAct->stringValue("content"));
		
		if(kvpSpeechAct->childForKey("in-reply-to")) {
		  oiIndividual.addDataProperty("knowrob:in-reply-to", "&xsd;string", kvpSpeechAct->stringValue("in-reply-to"));
		}
	      }
	      
	      std::string strDesigID = ckvpDesignator->stringValue("id");
	      
	      m_mapDesignators[strDesigID] = ckvpDesignator;
	      
	      if(strAnnotation == "parameter-annotation") { // Special treatment for parameter annotations
		KeyValuePair* ckvpChildren = ckvpDesignator->childForKey("description");
		
		if(ckvpChildren) {
		  for(KeyValuePair* ckvpChild : ckvpChildren->children()) {
		    std::string strKey = ckvpChild->key();
		    std::stringstream sts;
		    bool bSupportedType = true;
		    
		    switch(ckvpChild->type()) {
		    case KeyValuePair::ValueType::FLOAT:
		      sts << ckvpChild->floatValue();
		      break;
		      
		    case KeyValuePair::ValueType::STRING:
		      sts << ckvpChild->stringValue();
		      break;
		      
		    default:
		      this->warn("Unsupported parameter annotation type for key '" + strKey + "'.");
		      bSupportedType = false;
		      break;
		    }
		    
		    if(bSupportedType) {
		      if(find(m_lstAnnotatedParameters.begin(), m_lstAnnotatedParameters.end(), strKey) == m_lstAnnotatedParameters.end()) {
			m_lstAnnotatedParameters.push_back(strKey);
		      }
		      
		      std::string strNamespace = ckvpDesignator->stringValue("namespace");
		      
		      if(strNamespace == "") {
			strNamespace = "knowrob";
		      }
		      
		      oiIndividual.addContentProperty(strNamespace + ":" + strKey, sts.str());
		      oiIndividual.addDataProperty("knowrob:annotatedParameterType", "&xsd;string", strKey);
		    }
		  }
		}
		
		std::string strDesigPurpose = this->resolveDesignatorAnnotationTagName(strAnnotation);
		oiIndividual.addResourceProperty("knowrob:" + strDesigPurpose, "&" + strNamespace + ";" + strDesigID);
	      } else if(strAnnotation == "resource-annotation") {
		KeyValuePair* ckvpChildren = ckvpDesignator->childForKey("description");
		
		if(ckvpChildren) {
		  std::string strNamespace = ckvpChildren->stringValue("namespace");
		  std::string strProperty = ckvpChildren->stringValue("property");
		  std::string strURI = ckvpChildren->stringValue("uri");
		  
		  if(strNamespace == "") {
		    strNamespace = "knowrob";
		  }
		  
		  if(strProperty != "") {
		    if(strURI != "") {
		      oiIndividual.addResourceProperty(strNamespace + ":" + strProperty, strURI);
		    } else {
		      this->warn("Cannot annotate resource property: Field 'uri' missing.");
		    }
		  } else {
		    this->warn("Cannot annotate resource property: Field 'property' missing.");
		  }
		}		
	      }
	    }
	  }
	  
	  strDot += oiIndividual.print();
	  
	  ndLastDisplayed = ndCurrent;
	}
      } else {
	this->fail("Generation of event individual for node with invalid content requested!");
      }
    }
    
    return strDot;
  }
  
  std::string CExporterOwl::resolveDesignatorAnnotationTagName(std::string strAnnotation) {
    std::string strDesigPurpose = "";
    
    for(std::pair<std::string, std::string> prMapping : m_lstAnnotationPurposeMapping) {
      if(prMapping.first == strAnnotation) {
	strDesigPurpose = prMapping.second;
	break;
      }
    }
    
    if(strDesigPurpose == "") {
      strDesigPurpose = m_strDefaultAnnotation;
    }
    
    return strDesigPurpose;
  }
  
  std::string CExporterOwl::generateEventIndividuals(std::string strNamespace) {
    std::string strDot = "    <!-- Event Individuals -->\n\n";
    strDot += this->generateEventIndividualsForNodes(this->nodes(), strNamespace);
    
    return strDot;
  }
  
  std::string CExporterOwl::owlClassForObject(KeyValuePair *ckvpObject) {
    return "&knowrob;HumanScaleObject";
  }

  std::string CExporterOwl::owlClassForHuman(KeyValuePair *ckvpObject) {
    return "&knowrob;Person";
  }
  
  std::string CExporterOwl::failureClassForCondition(std::string strCondition) {
    std::string strFailureClass = m_strDefaultConditionMapping;
    
    for(std::pair<std::string, std::string> prCurrent : m_lstFailureMapping) {
      if(strCondition == prCurrent.first) {
	strFailureClass = prCurrent.second;
	break;
      }
    }
    
    return strFailureClass;
  }
  
  std::string CExporterOwl::generateFailureIndividualsForNodes(std::list<Node*> lstNodes, std::string strNamespace) {
    std::string strDot = "";
    
    for(Node* ndCurrent : lstNodes) {
      if(ndCurrent) {
	KeyValuePair *ckvpFailures = ndCurrent->metaInformation()->childForKey("failures");
	
	if(ckvpFailures) {
	  std::list<KeyValuePair*> lstFailures = ckvpFailures->children();
	  
	  unsigned int unIndex = 0;
	  for(KeyValuePair* ckvpFailure : lstFailures) {
	    std::stringstream sts;
	    sts << ndCurrent->uniqueID() << "_failure_" << unIndex;
	    
	    std::string strCondition = ckvpFailure->stringValue("condition");
	    std::string strTimestamp = ckvpFailure->stringValue("time-fail");
	    
	    std::string strFailureClass = this->failureClassForCondition(strCondition);
	    
	    OwlIndividual oiIndividual;
	    oiIndividual.setID("&" + strNamespace + ";" + sts.str());
	    oiIndividual.setType("&knowrob;" + strFailureClass);
	    oiIndividual.addDataProperty("rdfs:label", "&xsd;string", this->owlEscapeString(strCondition));
	    oiIndividual.addResourceProperty("knowrob:startTime", "&" + strNamespace + ";timepoint_" + strTimestamp);
	    
	    strDot += oiIndividual.print();
	  }
	}
	
	strDot += this->generateFailureIndividualsForNodes(ndCurrent->subnodes(), strNamespace);
      } else {
	this->fail("Failure node with invalid content!");
      }
    }
    
    return strDot;
  }
  
  std::string CExporterOwl::generateHumanIndividualsForNodes(std::list<Node*> lstNodes, std::string strNamespace) {
    std::string strDot = "";
    
    for(Node* ndCurrent : lstNodes) {
      if(ndCurrent) {
	KeyValuePair* ckvpHumans = ndCurrent->metaInformation()->childForKey("humans");
	
	if(ckvpHumans) {
	  std::list<KeyValuePair*> lstHumans = ckvpHumans->children();
	  
	  unsigned int unIndex = 0;
	  for(KeyValuePair* ckvpHuman : lstHumans) {
	    std::string strDesignatorID = ckvpHuman->stringValue("__id");
	    
	    std::string strDefClass = ckvpHuman->stringValue("_class");
	    std::string strDefClassNamespace = ckvpHuman->stringValue("_classnamespace");
	    
	    std::string strOwlClass = strDefClass;
	    if(strOwlClass == "") {
	      strOwlClass = this->owlClassForHuman(ckvpHuman);
	    } else {
	      strOwlClass = strDefClassNamespace + strDefClass;
	    }
	    
	    if(strDefClass == "") {
	      strDefClass = "human";
	    }
	    
	    if(strDefClassNamespace == "") {
	      strDefClassNamespace = "&" + strNamespace + ";";
	    }
	    
	    std::string strHumanID = strDefClass + "_" + ckvpHuman->stringValue("__id");
	    
	    if(find(m_lstExportedHumanIndividuals.begin(), m_lstExportedHumanIndividuals.end(), strHumanID) == m_lstExportedHumanIndividuals.end()) {
	      OwlIndividual oiIndividual;
	      oiIndividual.setID(strDefClassNamespace + strHumanID);
	      oiIndividual.setType(strOwlClass);
	      oiIndividual.addResourceProperty("knowrob:designator", "&" + strNamespace + ";" + strDesignatorID);

              if(ckvpHuman->childForKey("_srdlcomponent")) {
       	        oiIndividual.addResourceProperty("srdl2-comp:subComponent", ckvpHuman->stringValue("_srdlcomponent"));
              }
	      
	      if(ckvpHuman->childForKey("_tfprefix")) {
		oiIndividual.addDataProperty("srdl2-comp:tfPrefix", "&xsd;string", ckvpHuman->stringValue("_tfprefix"));
	      }
	      
	      strDot += oiIndividual.print();
	      
	      m_lstExportedHumanIndividuals.push_back(strHumanID);
	    }
	  }
	}
	
	strDot += this->generateHumanIndividualsForNodes(ndCurrent->subnodes(), strNamespace);
      } else {
	this->fail("Generation of human individual for node with invalid content requested!");
      }
    }
    
    return strDot;
  }
  
  std::string CExporterOwl::generateHumanIndividuals(std::string strNamespace) {
    std::string strDot = "    <!-- Human Individuals -->\n\n";
    strDot += this->generateHumanIndividualsForNodes(this->nodes(), strNamespace);
    
    return strDot;
  }

  std::string CExporterOwl::generateObjectIndividualsForNodes(std::list<Node*> lstNodes, std::string strNamespace) {
    std::string strDot = "";
    
    for(Node* ndCurrent : lstNodes) {
      if(ndCurrent) {
	KeyValuePair* ckvpObjects = ndCurrent->metaInformation()->childForKey("objects");
	
	if(ckvpObjects) {
	  std::list<KeyValuePair*> lstObjects = ckvpObjects->children();
	  
	  unsigned int unIndex = 0;
	  for(KeyValuePair* ckvpObject : lstObjects) {
	    std::string strDesignatorID = ckvpObject->stringValue("__id");
	    
	    std::string strDefClass = ckvpObject->stringValue("_class");
	    std::string strDefClassNamespace = ckvpObject->stringValue("_classnamespace");
	    
	    std::string strOwlClass = strDefClass;
	    if(strOwlClass == "") {
	      strOwlClass = this->owlClassForObject(ckvpObject);
	    } else {
	      strOwlClass = strDefClassNamespace + strDefClass;
	    }
	    
	    if(strDefClass == "") {
	      strDefClass = "object";
	    }
	    
	    if(strDefClassNamespace == "") {
	      strDefClassNamespace = "&" + strNamespace + ";";
	    }
	    
	    std::string strObjectID = strDefClass + "_" + ckvpObject->stringValue("__id");
	    
	    if(find(m_lstExportedObjectIndividuals.begin(), m_lstExportedObjectIndividuals.end(), strObjectID) == m_lstExportedObjectIndividuals.end()) {
	      OwlIndividual oiIndividual;
	      oiIndividual.setID(strDefClassNamespace + strObjectID);
	      oiIndividual.setType(strOwlClass);
	      oiIndividual.addResourceProperty("knowrob:designator", "&" + strNamespace + ";" + strDesignatorID);
	      
	      if(ckvpObject->childForKey("path-to-cad-model")) {
		oiIndividual.addDataProperty("knowrob:pathToCadModel", "&xsd;string", ckvpObject->stringValue("path-to-cad-model"));
	      }
	      
	      strDot += oiIndividual.print();
	      
	      m_lstExportedObjectIndividuals.push_back(strObjectID);
	    }
	  }
	}
	
	strDot += this->generateObjectIndividualsForNodes(ndCurrent->subnodes(), strNamespace);
      } else {
	this->fail("Generation of object individual for node with invalid content requested!");
      }
    }
    
    return strDot;
  }
  
  std::string CExporterOwl::generateObjectIndividuals(std::string strNamespace) {
    std::string strDot = "    <!-- Object Individuals -->\n\n";
    strDot += this->generateObjectIndividualsForNodes(this->nodes(), strNamespace);
    
    return strDot;
  }
  
  std::string CExporterOwl::generateImageIndividualsForNodes(std::list<Node*> lstNodes, std::string strNamespace) {
    std::string strDot = "";
    
    for(Node* ndCurrent : lstNodes) {
      if(ndCurrent) {
	KeyValuePair* ckvpImages = ndCurrent->metaInformation()->childForKey("images");
	
	if(ckvpImages) {
	  std::list<KeyValuePair*> lstImages = ckvpImages->children();
	  
	  unsigned int unIndex = 0;
	  for(KeyValuePair* ckvpImage : lstImages) {
	    std::stringstream sts;
	    sts << ndCurrent->uniqueID() << "_image_" << unIndex;
	    
	    std::string strOwlClass = "&knowrob;CameraImage";
	    std::string strFilename = ckvpImage->stringValue("filename");
	    std::string strTopic = ckvpImage->stringValue("origin");
	    std::string strCaptureTime = ckvpImage->stringValue("time-capture");
	    
	    OwlIndividual oiIndividual;
	    oiIndividual.setID("&" + strNamespace + ";" + sts.str());
	    oiIndividual.setType(strOwlClass);
	    oiIndividual.addDataProperty("knowrob:linkToImageFile", "&xsd;string", strFilename);
	    oiIndividual.addDataProperty("knowrob:rosTopic", "&xsd;string", strTopic);
	    oiIndividual.addResourceProperty("knowrob:captureTime", "&" + strNamespace + ";timepoint_" + strCaptureTime);
	    
	    strDot += oiIndividual.print();
	  }
	}
	
	strDot += this->generateImageIndividualsForNodes(ndCurrent->subnodes(), strNamespace);
      } else {
	this->fail("Image node with invalid content!");
      }
    }
    
    return strDot;
  }
  
  std::string CExporterOwl::generateImageIndividuals(std::string strNamespace) {
    std::string strDot = "    <!-- Image Individuals -->\n\n";
    
    strDot += this->generateImageIndividualsForNodes(this->nodes(), strNamespace);
    
    return strDot;
  }
  
  std::string CExporterOwl::generateDesignatorIndividuals(std::string strNamespace) {
    std::string strDot = "    <!-- Designator Individuals -->\n\n";
    
    std::list<std::string> lstDesigIDs = this->designatorIDs();
    
    for(std::string strID : lstDesigIDs) {
      OwlIndividual oiIndividual;
      oiIndividual.setID("&" + strNamespace + ";" + strID);

      std::string strDesignatorClass = m_strDefaultDesignatorClass;
      
      if(m_mapDesignators.find(strID) != m_mapDesignators.end()) {
	if(m_mapDesignators[strID]) {
	  if(m_mapDesignators[strID]->childForKey("type")) {
	    if(m_mapDesignators[strID]->stringValue("type") == "ACTION") {
	      strDesignatorClass = "CRAMActionDesignator";
	    } else if(m_mapDesignators[strID]->stringValue("type") == "LOCATION") {
	      strDesignatorClass = "CRAMLocationDesignator";
	    } else if(m_mapDesignators[strID]->stringValue("type") == "HUMAN") {
	      strDesignatorClass = "CRAMHumanDesignator";
	    } else if(m_mapDesignators[strID]->stringValue("type") == "OBJECT") {
	      strDesignatorClass = "CRAMObjectDesignator";
	    }
	  }
	}
      }
      
      oiIndividual.setType("&knowrob;" + strDesignatorClass);
      
      if(m_mapDesignators.find(strID) != m_mapDesignators.end()) {
	std::string strTimeCreated = m_mapDesignators[strID]->childForKey("description")->stringValue("_time_created");
	oiIndividual.addResourceProperty("knowrob:creationTime", "&" + strNamespace + ";timepoint_" + strTimeCreated);
      }
      
      std::list<std::string> lstSuccessorIDs = this->successorDesignatorsForID(strID);
      for(std::string strID2 : lstSuccessorIDs) {
	oiIndividual.addResourceProperty("knowrob:successorDesignator", "&" + strNamespace + ";" + strID2);
      }
      
      std::string strEquationTime = this->equationTimeForSuccessorID(strID);
      if(strEquationTime != "") {
	oiIndividual.addResourceProperty("knowrob:equationTime", "&" + strNamespace + ";timepoint_" + strEquationTime);
      }
      
      // NOTE(winkler): This is an index designator (i.e. `first in
      // chain') when a) there are successors, and b) it does not have
      // an equation time.
      if(lstSuccessorIDs.size() > 0 && strEquationTime == "") {
	strDot += "    <!-- This is an index designator -->\n";
	
	std::list<std::string> lstAllSuccessors = this->collectAllSuccessorDesignatorIDs(strID);
	for(std::string strSuccessor : lstAllSuccessors) {
	  oiIndividual.addResourceProperty("knowrob:equatedDesignator", "&" + strNamespace + ";" + strSuccessor);
	}
      }
      
      strDot += oiIndividual.print();
    }
    
    return strDot;
  }
  
  std::list<std::string> CExporterOwl::collectAllSuccessorDesignatorIDs(std::string strDesigID) {
    std::list<std::string> lstReturn;
    std::list<std::string> lstSuccessors = this->successorDesignatorsForID(strDesigID);
    
    for(std::string strSuccessor : lstSuccessors) {
      std::list<std::string> lstSubSuccessors = this->collectAllSuccessorDesignatorIDs(strSuccessor);
      
      for(std::string strSubSuccessor : lstSubSuccessors) {
	lstReturn.push_back(strSubSuccessor);
      }
      
      lstReturn.push_back(strSuccessor);
    }
    
    return lstReturn;
  }
  
  std::string CExporterOwl::generateFailureIndividuals(std::string strNamespace) {
    std::string strDot = "    <!-- Failure Individuals -->\n\n";
    strDot += this->generateFailureIndividualsForNodes(this->nodes(), strNamespace);
    
    return strDot;
  }
  
  std::string CExporterOwl::generateTimepointIndividuals(std::string strNamespace) {
    std::string strDot = "    <!-- Timepoint Individuals -->\n\n";
    
    std::list<Node*> lstTrace;
    std::list<std::string> lstTimepoints = this->gatherTimepointsForNodes(this->nodes(), lstTrace);
    
    // Unify all timepoints
    std::list<std::string> lstTimepointsUnified;
    this->info("      Acquired " + this->str((int)lstTimepoints.size()) + " timepoint(s) for unification.");
    
    for(std::string strTimepoint : lstTimepoints) {
      if(std::find(lstTimepointsUnified.begin(), lstTimepointsUnified.end(), strTimepoint) == lstTimepointsUnified.end()) {
	lstTimepointsUnified.push_back(strTimepoint);
      }
    }
    
    this->info("      Unification complete.");
    
    for(std::string strTimepoint : lstTimepointsUnified) {
      OwlIndividual oiIndividual;
      oiIndividual.setID("&" + strNamespace + ";timepoint_" + strTimepoint);
      oiIndividual.setType("&knowrob;TimePoint");
      
      strDot += oiIndividual.print();
      
      // Find earliest and latest timepoint
      if(m_mapMetaData.find("time-start") == m_mapMetaData.end()) {
	m_mapMetaData["time-start"] = {MappedMetaData::Property, strTimepoint};
      } else {
	float fTimeNew, fTimeOld;
	sscanf(strTimepoint.c_str(), "%f", &fTimeNew);
	sscanf(m_mapMetaData["time-start"].strValue.c_str(), "%f", &fTimeOld);
	
	if(fTimeNew < fTimeOld) {
	  m_mapMetaData["time-start"] = {MappedMetaData::Property, strTimepoint};
	}
      }
      
      if(m_mapMetaData.find("time-end") == m_mapMetaData.end()) {
	m_mapMetaData["time-end"] = {MappedMetaData::Property, strTimepoint};
      } else {
	float fTimeNew, fTimeOld;
	sscanf(strTimepoint.c_str(), "%f", &fTimeNew);
	sscanf(m_mapMetaData["time-end"].strValue.c_str(), "%f", &fTimeOld);
	
	if(fTimeNew > fTimeOld) {
	  m_mapMetaData["time-end"] = {MappedMetaData::Property, strTimepoint};
	}
      }
    }
    
    return strDot;
  }
  
  std::string CExporterOwl::generateMetaDataIndividual(std::string strNamespace) {
    std::string strDot = "    <!-- Meta Data Individual -->\n\n";
    std::string strUniqueName = this->generateUniqueID("RobotExperiment_");
    
    OwlIndividual oiIndividual;
    oiIndividual.setID("&" + strNamespace + ";" + strUniqueName);
    oiIndividual.setType("&knowrob;RobotExperiment");
    
    std::list<Node*> lstRootNodes = this->rootNodes();
    for(Node* ndRoot : lstRootNodes) {
      oiIndividual.addResourceProperty("knowrob:subAction", "&" + strNamespace + ";" + ndRoot->uniqueID());
    }
    
    for(std::pair<std::string, MappedMetaData> prEntry : m_mapMetaData) {
      std::string strCamelCaseKey = prEntry.first;
      int nCharCount = prEntry.first.length();
      
      for(int nI = 0; nI < nCharCount; nI++) {
	if(strCamelCaseKey[nI] == '-') {
	  std::string strTemp = strCamelCaseKey.substr(nI + 1, 1);
	  transform(strTemp.begin(), strTemp.end(), strTemp.begin(), ::toupper);
	  strCamelCaseKey.erase(nI, 2);
	  strCamelCaseKey.insert(nI, strTemp);
	  nCharCount--;
	}
      }
      
      if(strCamelCaseKey == "timeEnd" || strCamelCaseKey == "timeStart") {
	// Special handling for timepoints as they mark specific timepoint individuals
	oiIndividual.addResourceProperty("knowrob:" + (strCamelCaseKey == "timeStart" ? std::string("startTime") : std::string("endTime")), "&" + strNamespace + ";" + "timepoint_" + std::string(prEntry.second.strValue));
      } else {
	if(prEntry.second.tpType == MappedMetaData::Resource) {
	  std::string strUseNamespace = "";
	  if(!prEntry.second.bIgnoreNamespace) {
	    strUseNamespace = "&" + strNamespace + ";";
	  }
	  
	  oiIndividual.addResourceProperty("knowrob:" + strCamelCaseKey, strUseNamespace + prEntry.second.strValue);
	} else {
	  oiIndividual.addDataProperty("knowrob:" + strCamelCaseKey, "&xsd;string", prEntry.second.strValue);
	}
      }
    }
    
    strDot += oiIndividual.print();
    
    return strDot;
  }
  
  std::string CExporterOwl::generateParameterAnnotationInformation(std::string strNamespace) {
    std::string strDot = "    <!-- Parameter Annotation Information Individual -->\n\n";
    std::string strUniqueName = this->generateUniqueID("AnnotationInformation_");
    
    OwlIndividual oiIndividual;
    oiIndividual.setID("&" + strNamespace + ";" + strUniqueName);
    oiIndividual.setType("&knowrob;AnnotationInformation");
    
    for(std::string strParameterAnnotation : m_lstAnnotatedParameters) {
      oiIndividual.addDataProperty("knowrob:annotatedParameterType", "&xsd;string", strParameterAnnotation);
    }
    
    strDot += oiIndividual.print();
    
    return strDot;
  }
  
  std::string CExporterOwl::owlClassForNode(Node *ndNode, bool bClassOnly, bool bPrologSyntax) {
    std::string strName = "";
    
    if(ndNode) {
      strName = ndNode->title();
    }
    
    std::string strPlainPrefix = "knowrob";
    std::string strClass = "CRAMAction";
    
    if(strName == "WITH-DESIGNATORS") {
      strClass = "WithDesignators";
    } else if(strName == "SPEECH-ACT") {
      strClass = "SpeechAct";
    } else if(strName == "OPEN-GRIPPER") {
      //strClass = "CRAMGripperCommand";
      strClass = "OpeningAGripper";
    } else if(strName == "CLOSE-GRIPPER") {
      //strClass = "CRAMGripperCommand";
      strClass = "ClosingAGripper";
    } else if(strName == "TAG") {
      strClass = "CRAMPlanTag";
    } else if(strName.substr(0, 5) == "GOAL-") {
      // This is a goal definition.
      std::string strGoal = strName.substr(5);
      
      // Missing yet:
      /*
	PREVENT
	MAINTAIN
	INFORM (add information to belief state from outside)
      */
      
      if(strGoal == "PERCEIVE-OBJECT") {
	strClass = "CRAMPerceive";
      } else if(strGoal == "ACHIEVE") {
	strClass = "CRAMAchieve";
      } else if(strGoal == "PERFORM") { // Should go into another structure (?)
	strClass = "CRAMPerform";
      } else if(strGoal == "MONITOR-ACTION") {
	strClass = "CRAMMonitor";
      } else if(strGoal == "PERFORM-ON-PROCESS-MODULE") {
	strClass = "PerformOnProcessModule";
      } else {
	strClass = "DeclarativeGoal";
      }
    } else if(strName.substr(0, 8) == "RESOLVE-") {
      // This is a designator resolution.
      std::string strDesigType = strName.substr(8);
      
      if(strDesigType == "LOCATION-DESIGNATOR") {
	strClass = "ResolveLocationDesignator";
      } else if(strDesigType == "ACTION-DESIGNATOR") {
	strClass = "ResolveActionDesignator";
      }
    } else if(strName.substr(0, 21) == "REPLACEABLE-FUNCTION-") {
      // This is an internal function name
      std::string strFunction = strName.substr(21);
      
      if(strFunction == "NAVIGATE") {
	strClass = "BaseMovement"; // NOTE(winkler): was 'Navigate'
      }
    } else if(strName.substr(0, 8) == "PERFORM-") {
      // This is the performance of probably a designator
      std::string strPerformer = strName.substr(8);
      
      if(strPerformer == "ACTION-DESIGNATOR") {
	KeyValuePair* ckvpDescription = NULL;
	std::list<KeyValuePair*> lstDesc = ndNode->description();
	
	for(KeyValuePair* ckvpCurrent : lstDesc) {
	  if(ckvpCurrent->key() == "DESCRIPTION") {
	    ckvpDescription = ckvpCurrent;
	    break;
	  }
	}
	
	bool bSpecializedDesignator = true;
	if(ckvpDescription) {
	  std::string strTo = ckvpDescription->stringValue("TO");
	  std::string strType = ckvpDescription->stringValue("TYPE");
	  
	  if(strTo == "GRASP") {
	    // Specializer: Grasping.
	    strClass = "PickingUpAnObject";
	  } else if(strTo == "LIFT") {
	    // Specializer: Lifting.
	    strClass = "LiftingAnObject";
	  } else if(strTo == "CARRY") {
	    // Specializer: Carrying.
	    strClass = "CarryingAnObject";
	  } else if(strTo == "PERCEIVE") {
	    // Specializer: Perceiving.
	    strClass = "VisualPerception";
	  } else if(strTo == "PUT-DOWN") {
	    // Specializer: Putting down.
	    strClass = "PuttingDownAnObject";
	  } else if(strTo == "PARK") {
	    // Specializer: Putting down.
	    strClass = "ParkingArms";
	  } else if(strTo == "REACH") {
	    // Specializer: Reaching.
	    strClass = "Reaching";
	  } else if(strTo == "RELEASE") {
	    // Specializer: Releasing.
	    strClass = "ReleasingTheGraspOfSomething";
	  } else if(strTo == "CLAMP") {
	    // Specializer: Clamping.
	    strClass = "GraspingSomething";
	  } else if((strTo == "OPEN") && 
                    (ckvpDescription->stringValue("BODYPART") == "GRIPPER")) {
	    // Specializer: Opening Gripper.
	    strClass = "OpeningAGripper";
	  } else if(strTo == "MOVE") {
	    // Specializer: Moving.
	    strClass = "VoluntaryBodyMovement";
	  } else if(strType == "NAVIGATION") {
	    // Specializer: Navigating.
	    strClass = "Navigate";
	  } else {
	    // Fallback.
            std::cout << "\n\n No suitable description for perform-action-designator: " << strTo << " \n\n" << std::endl;
	    
	    bSpecializedDesignator = false;
	  }
	} else {
	  // Fallback.
          std::cout << "\n\n No description for perform-action-designator! \n\n" << std::endl;
	  bSpecializedDesignator = false;
	}
	
	if(bSpecializedDesignator == false) {
	  // Default class if no specializer could be found.
	  strClass = "PerformActionDesignator";
	}
      }
    } else if(strName == "UIMA-PERCEIVE") {
      strClass = "UIMAPerception";
    } else if(strName == "GRASP-OBJECT") {
      strClass = "PickingUpAnObject";
    } else if(strName == "PUT-DOWN-OBJECT") {
      strClass = "PuttingDownAnObject";
    } else if(strName == "FIND-OBJECTS") {
      strClass = "FindingObjects";
    } else if(strName == "OBJECT-IDENTITY-RESOLUTION") {
      strClass = "ObjectIdentityResolution";
    } else if(strName == "BELIEF-STATE-UPDATE") {
      strClass = "BeliefStateUpdate";
    } else if(strName == "MOTION-PLANNING") {
      strClass = "MotionPlanning";
    } else if(strName == "MOTION-EXECUTION") {
      strClass = "MotionExecution";
    } else if(strName == "AT-LOCATION") {
      strClass = "AtLocation";
    } else if(strName == "VOLUNTARY-BODY-MOVEMENT-ARMS") {
      strClass = "ArmMovement";
    } else if(strName == "VOLUNTARY-BODY-MOVEMENT-HEAD") {
      strClass = "HeadMovement";
    } else if(strName == "WITH-FAILURE-HANDLING") {
      strClass = "WithFailureHandling";
    } else if(strName == "WITH-POLICY") {
      strClass = "WithPolicy";
    } else if(strName == "SLICING") {
      strPlainPrefix = "knowrob";
      strClass = "Slicing";
    } else if(strName == "PERCEIVE-OBJECT") {
      strPlainPrefix = "knowrob_cram";
      strClass = "PerceiveObject";
    } else if(strName == "HUMAN-INTRUSION") {
      strPlainPrefix = "saphari";
      strClass = "HumanIntrusion";
    } else if(strName == "PERCEIVE-HUMAN") {
      strPlainPrefix = "knowrob_cram";
      strClass = "PerceivePerson";
    } else if(strName == "LOWER_OBJECT") {
      strClass = "LoweringAnObject";
    }
    
    std::string strPrefix = (bPrologSyntax ? strPlainPrefix + ":" : "&" + strPlainPrefix + ";");
    
    return (bClassOnly ? "" : strPrefix) + (bPrologSyntax ? "'" + strClass + "'" : strClass);
  }
  
  bool CExporterOwl::runExporter(KeyValuePair* ckvpConfigurationOverlay) {
    m_lstAnnotatedParameters.clear();
    m_lstExportedObjectIndividuals.clear();
    m_lstExportedHumanIndividuals.clear();
   
    m_nThrowAndCatchFailureCounter = 0;
    
    this->info("Renewing unique IDs");
    this->renewUniqueIDs();
    
    this->info("Generating XML");
    if(this->outputFilename() != "") {
      std::string strOwl = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\n";
      // NOTE(winkler): This used to be `random'. Changed this due to
      // non-necessity of such a long namespace.
      // this->generateRandomIdentifier("namespace_", 8);
      std::string strNamespaceID = "log";
      std::string strNamespace = "http://knowrob.org/kb/cram_log.owl";// + strNamespaceID;
      
      // Prepare content
      this->info(" - Preparing content");
      this->prepareEntities(strNamespaceID, strNamespace);
      
      // Generate source
      this->info(" - Generating source");
      strOwl += this->generateOwlStringForNodes(this->nodes(), strNamespaceID, strNamespace);
      
      // Write the .owl file
      this->info(" - Writing file");
      return this->writeToFile(strOwl);
    } else {
      this->fail("No output filename was given. Cancelling.");
    }
    
    return false;
  }
  
  std::string CExporterOwl::owlEscapeString(std::string strValue) {
    return strValue;
  }
  
  std::string CExporterOwl::generateOwlStringForNodes(std::list<Node*> lstNodes, std::string strNamespaceID, std::string strNamespace) {
    std::string strOwl = "";
    OwlIndividual::resetIssuedInformation();
    
    // Assemble OWL source
    this->info("   - Block: DocType");
    strOwl += this->generateDocTypeBlock();
    this->info("   - Block: XMLNS");
    strOwl += this->generateXMLNSBlock(strNamespace);
    this->info("   - Block: Imports");
    strOwl += this->generateOwlImports(strNamespace);
    
    std::string strOwl2 = "";
    this->info("   - Block: Event Individuals");
    strOwl2 += this->generateEventIndividuals(strNamespaceID);
    this->info("   - Block: Object Individuals");
    strOwl2 += this->generateObjectIndividuals(strNamespaceID);
    this->info("   - Block: Human Individuals");
    strOwl2 += this->generateHumanIndividuals(strNamespaceID);
    this->info("   - Block: Image Individuals");
    strOwl2 += this->generateImageIndividuals(strNamespaceID);
    this->info("   - Block: Designator Individuals");
    strOwl2 += this->generateDesignatorIndividuals(strNamespaceID);
    this->info("   - Block: Failure Individuals");
    strOwl2 += this->generateFailureIndividuals(strNamespaceID);
    this->info("   - Block: Timepoint Individuals (this can take a while)");
    strOwl2 += this->generateTimepointIndividuals(strNamespaceID);
    this->info("   - Block: Meta Data Individual");
    strOwl2 += this->generateMetaDataIndividual(strNamespaceID);
    this->info("   - Block: Parameter Annotations");
    strOwl2 += this->generateParameterAnnotationInformation(strNamespaceID);
    strOwl2 += "</rdf:RDF>\n";
    
    // NOTE(winkler): Doing this afterwards, as the other blocks being
    // placed after them in the file actually issue classes and
    // properties that need to be mentioned in the definitions (which
    // are placed further up in the file). That's why we're shifting
    // order here.
    this->info("   - Block: Property Definitions");
    strOwl += this->generatePropertyDefinitions();
    this->info("   - Block: Class Definitions");
    strOwl += this->generateClassDefinitions();
    
    strOwl += strOwl2;
    
    if(m_nThrowAndCatchFailureCounter > 0) {
      this->warn("Throw/Catch failure counter is > 0: '" + this->str(m_nThrowAndCatchFailureCounter) + "'");
    }
    
    return strOwl;
  }
}
