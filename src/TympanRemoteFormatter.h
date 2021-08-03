#ifndef TympanRemoteFormatter_h
#define TympanRemoteFormatter_h

#define TR_MAX_N_PAGES 10
#define TR_MAX_N_CARDS 10
#define TR_MAX_N_BUTTONS 12

#include <Arduino.h>

class TR_Button {
  public:
    String label;
    String command;
    String id;
    int width;

    TR_Button() {}

    String asString();
	
	TR_Button *prevButton = NULL;  //for linked list
	TR_Button *nextButton = NULL;  //for linked list
};


class TR_Card {
  
  public:
    String name = "";

    TR_Card(void) { }
	~TR_Card(void) {
		//loop through and delete each button that was created
		TR_Button *button = lastButton;
		while (button != NULL) {
			TR_Button *prev_button = button->prevButton; //look to the next loop
			delete button;   //delete the button
			nButtons--;		 //decrement the count
			
			button = prev_button; //prepare for the next loop
			if (button != NULL) button->nextButton = NULL;  //there is no more next button
			lastButton = button;  //update what the new lastButton is
		}
	}

    TR_Button* addButton(String label, String command, String id, int width);
	TR_Button* getFirstButton(void);
	
    String asString(void);

	//TR_Button *firstButton = NULL;
	TR_Button *lastButton = NULL;   //for linked list
	TR_Card *prevCard = NULL;       //for linked list
	TR_Card *nextCard = NULL; 		//for linked list

  private:
    //TR_Button buttons[TR_MAX_N_BUTTONS];
    int nButtons = 0;
};


class TR_Page {
  public:

    String name = "";
    
    TR_Page(void) {}
	~TR_Page(void) {
		//loop through and delete each card that was created
		TR_Card *card = lastCard;
		while (card != NULL) {
			TR_Card *prev_card = card->prevCard; //look to the next loop
			delete card;  //delete the card
			nCards--;     //decrement the count
			
			card = prev_card; //prepare for next loop
			if (card != NULL) card->nextCard = NULL;  //there is no more next card
			lastCard = card;  //update what the new lastCard is
		}
	}


//	TR_Card *addCard(TR_Card *_inCard) {
//		if (nCards >= TR_MAX_N_CARDS) return NULL;  //too many already!
//		cards[nCards] = *_inCard;  //copy
//		nCards++; //increment to next card
//		return _inCard;
//	}

    
	TR_Card* addCard(String name);
	TR_Card* getFirstCard(void);
	
    String asString(void);
	
	//TR_Card *firstCard = NULL;
	TR_Card *lastCard = NULL;   //for linked list
	TR_Page *prevPage = NULL;   //for linked list
	TR_Page *nextPage = NULL;   //for linked list
	
  private:
    //TR_Card cards[TR_MAX_N_CARDS];
    int nCards = 0;
};


class TympanRemoteFormatter {
  public:
    TympanRemoteFormatter(void) { }
	~TympanRemoteFormatter(void) { deleteAllPages(); }

//	TR_Page* addPage(TR_Page *_inPage) {
//		if (nPages >= TR_MAX_N_PAGES)  return NULL;
//		pages[nPages] = *_inPage;  //copy
//		nPages++;
//		return _inPage;
//	}

    TR_Page* addPage(String name);
    void addPredefinedPage(String s);
	TR_Page* getFirstPage(void);
	
	//int c_str(char *c_str, int maxLen);
    String asString(void);
    void asValidJSON(void);

	int get_nPages(void) { return nCustomPages + nPredefinedPages; }
	void deleteAllPages(void);
	
	//TR_Page *firstPage = NULL;
	TR_Page *lastPage = NULL;    //for linked list

  private:
    //TR_Page pages[TR_MAX_N_PAGES];
    //int nPages = 0;
    int nCustomPages = 0;
	int nPredefinedPages = 0;
	String predefPages;
};




#endif
