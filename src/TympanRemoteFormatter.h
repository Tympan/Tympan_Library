#ifndef TympanRemoteFormatter_h
#define TympanRemoteFormatter_h

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
      s = "{'label':'"+label+"','cmd':'"+command+"','id':'"+id+"','width':'"+width+"'}";
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

  private:
    TR_Button buttons[10];
    int nButtons;
};

class TR_Page {
  public:

    String name = "";
    
    TR_Page() {
      nCards = 0;
    }

    TR_Card* addCard(String name) {
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

  private:
    TR_Card cards[10];
    int nCards;
};

class TympanRemoteFormatter {
  public:
    TympanRemoteFormatter(void) {
      nPages = 0;
    }

    TR_Page* addPage(String name) {
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
    
    String asString()  {
      String s;
      int i;

      s = "JSON={'icon':'tympan.png',";

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
        s += "'prescription':{'type':'BoysTown','pages':["+ predefPages +"]}";
      }
      s += "}";
      
      return s;      
    }

    void asValidJSON() {
      String s;
      s = this->asString();
      s.replace("'","\"");
      Serial.println("Pretty printed (a.k.a. valid json):");
      Serial.println(s);
    }

  private:
    TR_Page pages[10];
    int nPages;
    String predefPages;
};

#endif
