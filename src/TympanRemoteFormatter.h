#ifndef TympanRemoteFormatter_h
#define TympanRemoteFormatter_h

#define TR_MAX_N_PAGES 10
#define TR_MAX_N_CARDS 10
#define TR_MAX_N_BUTTONS 10

class TR_Button {
  public:
    String label;
    String command;
    String id;
    int width;

    TR_Button() {
    }

    String asString() {
		String s;
		
		//original method
		//s = "{'label':'" + label + "','cmd':'" + command + "','id':'" + id + "','width':'" + width + "'}";
		
		//new method...don't waste time transmitting fields that don't exist
		s = "{";
		if (label.length() > 0) s.concat("'label':'" + label + "',");
		if (command.length() > 0) s.concat("'cmd':'" + command + "',");
		if (id.length() > 0) s.concat("'id':'" + id + "',");
		//if (width.length() > 0) s.concat("'width':'" + width + "'"); //no trailing comma
		s.concat("'width':'" + String(width) + "'"); //always have a width
		
		//if (s.endsWith(",")) s.remove(s.length()-1,1); //strip off trailing comma
		s.concat("}"); //close off the unit
		
		return s;
    }
};

class TR_Card {
  
  public:
    String name = "";

    TR_Card() {
      nButtons = 0;
    }

    void addButton(String label, String command, String id, int width) {
      TR_Button *btn;

	  if (nButtons >= TR_MAX_N_BUTTONS) return; //too many buttons already!
      btn = &(buttons[nButtons]);
      btn->label = label;
      btn->command = command;
      btn->id = id;
      btn->width = width;

      nButtons++;
    }

    String asString() {
      String s;
      int i;
      
      s = "{'name':'"+name+"'";
      if (nButtons > 0) {
        s += ",'buttons':[";
        s += buttons[0].asString();
      }
      for (i=1; i<nButtons; i++) {
        s += ",";
        s += buttons[i].asString();        
      }
      if (nButtons > 0) {
        s += "]";        
      }
      s += "}";

      return s;
    }

	String getName(void) { return name; }
	void setName(String s) { name = s; }

  private:
    TR_Button buttons[TR_MAX_N_BUTTONS];
    int nButtons;
};

class TR_Page {
  public:

    String name = "";
    
    TR_Page() {
      nCards = 0;
    }

//	TR_Card *addCard(TR_Card *_inCard) {
//		if (nCards >= TR_MAX_N_CARDS) return NULL;  //too many already!
//		cards[nCards] = *_inCard;  //copy
//		nCards++; //increment to next card
//		return _inCard;
//	}
    TR_Card *addCard(String name) {
		if (nCards >= TR_MAX_N_CARDS) return NULL;  //too many already!
		TR_Card *card;

		card = &(cards[nCards]);
		card->name = name;
		nCards++;

		return card;
    }

    String asString() {
      String s;
      int i;
      
      s = "{'title':'"+name+"'";
      if (nCards > 0) {
        s += ",'cards':[";
        s += cards[0].asString();
      }
      for (i=1; i<nCards; i++) {
        s += ",";
        s += cards[i].asString();        
      }
      if (nCards > 0) {
        s += "]";        
      }
      s += "}";
      
      return s;
    }
	
	String getName(void) { return name; }
	void setName(String s) { name = s; }

  private:
    TR_Card cards[TR_MAX_N_CARDS];
    int nCards;
};

class TympanRemoteFormatter {
  public:
    TympanRemoteFormatter(void) {
      nPages = 0;
    }

//	TR_Page* addPage(TR_Page *_inPage) {
//		if (nPages >= TR_MAX_N_PAGES)  return NULL;
//		pages[nPages] = *_inPage;  //copy
//		nPages++;
//		return _inPage;
//	}
    TR_Page* addPage(String name) {
		if (nPages >= TR_MAX_N_PAGES)  return NULL;
		TR_Page *page;
		page = &(pages[nPages]);
		page->name = name;
		nPages++;
		return page;
    } 

    void addPredefinedPage(String s) {
      if (predefPages.length()>0) {
        predefPages += ",";
      }
      predefPages += "'"+s+"'";
    }
	
	int c_str(char *c_str, int maxLen) {
		int dest=0, source=0;
		String s;
		
		s = String(F("JSON={'icon':'tympan.png',"));
		source = 0;
		while ((dest < maxLen) && (source < (int)s.length())) c_str[dest++] = s[source++];
		
		// Add pages:
		if (nPages > 0) {
			s = String("'pages':[") + pages[0].asString();
			source = 0;	while ((dest < maxLen) && (source < (int)s.length())) c_str[dest++] = s[source++];
		}
		for (int i=1; i<nPages; i++) {
			s  = String(",") + pages[i].asString();
			source = 0;	while ((dest < maxLen) && (source < (int)s.length())) c_str[dest++] = s[source++];
		}
		if (nPages > 0) {
			s = String("]");        
			source = 0;	while ((dest < maxLen) && (source < (int)s.length())) c_str[dest++] = s[source++];
		}
		
		// Add predefined pages:
		if (predefPages.length()>1) {
			if (dest>0) {
				s = String(",");        
				source = 0;	while ((dest < maxLen) && (source < (int)s.length())) c_str[dest++] = s[source++];
			}
			s = String(F("'prescription':{'type':'BoysTown','pages':[")) + predefPages + String("]}");
			source = 0;	while ((dest < maxLen) && (source < (int)s.length())) c_str[dest++] = s[source++];
		}
		s = String("}");
		source = 0;	while ((dest < maxLen) && (source < (int)s.length())) c_str[dest++] = s[source++];
		
		dest = min(dest,maxLen);
		c_str[dest] = '\0';  //null terminated
		return dest;
	}
    
    String asString()  {
      String s;
      int i;

      s = F("JSON={'icon':'tympan.png',");

      // Add pages:
      if (nPages > 0) {
        s += "'pages':[";
        s += pages[0].asString();
      }
      for (i=1; i<nPages; i++) {
        s += ",";
        s += pages[i].asString();        
      }
      if (nPages > 0) {
        s += "]";        
      }
      // Add predefined pages:
      if (predefPages.length()>1) {
        if (s.length()>1) {
          s += ",";        
        }
        s += F("'prescription':{'type':'BoysTown','pages':[") + predefPages +"]}";
      }
      s += "}";
      
      return s;      
    }

    void asValidJSON() {
      String s;
      s = this->asString();
      s.replace("'","\"");
      Serial.println(F("Pretty printed (a.k.a. valid json):"));
      Serial.println(s);
    }

	int get_nPages(void) { return nPages; }

  private:
    TR_Page pages[TR_MAX_N_PAGES];
    int nPages;
    String predefPages;
};




#endif
