
#include <TympanRemoteFormatter.h>

String TR_Button::asString(void) {
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



// ////////////////////////////////////////////////////////////////////////////////////////////////////////


TR_Button* TR_Card::addButton(String label, String command, String id, int width) {
	//make a new button
	TR_Button *btn = new TR_Button();
	if (btn==NULL) return btn; //return if we can't allocate it
	
	//link it to the previous button (linked list)
	TR_Button *prevLastButton = lastButton;
	if (prevLastButton != NULL) prevLastButton->nextButton = btn;
	
	//assign the new page to be the new last page
	lastButton = btn;
	nButtons++;
	
	//add the data
	btn->label = label;
	btn->prevButton = prevLastButton;
	btn->command = command;
	btn->id = id;
	btn->width = width;

	//we're done
	return btn;	
}

TR_Button* TR_Card::getFirstButton(void) {
	TR_Button *btn = lastButton;
	if (btn == NULL) return NULL;

	//keep stepping back to find the first
	TR_Button *prev_btn = btn->prevButton;
	while (prev_btn != NULL) {
		btn = prev_btn;
		prev_btn = btn->prevButton;
	}
	return btn;
}

String TR_Card::asString(void) {
	String s;

	s = "{'name':'"+name+"'";
	
	//write buttons
	TR_Button* btn = getFirstButton();
	if (btn != NULL) {
		//write the first button
		s += ",'buttons':[";
		s += btn->asString();

		//write other buttons
		TR_Button* next_button = btn->nextButton;
		while (next_button != NULL) {
			btn = next_button;
			s += ",";
			s += btn->asString();        
			next_button = btn->nextButton;
		}
		
		//finish the list
		s += "]";        
	}
	
	//close out
	s += "}";

	return s;
}


// ////////////////////////////////////////////////////////////////////////////////////////////////////////

TR_Card* TR_Page::addCard(String name) {

	//make a new card
	TR_Card *card = new TR_Card();
	if (card==NULL) return card; //return if we can't allocate it
	
	//link it to the previous card (linked list)
	TR_Card *prevLastCard = lastCard;
	if (prevLastCard != NULL) prevLastCard->nextCard = card;
	
	//assign the new page to be the new last page
	lastCard = card;
	nCards++;
	
	//add the data
	card->name = name;
	card->prevCard = prevLastCard;

	//we're done
	return card;
}

TR_Card* TR_Page::getFirstCard(void) {
	TR_Card *card = lastCard;
	if (card == NULL) return NULL;
	
	//keep stepping back to find the first
	TR_Card *prev_card = card->prevCard;
	while (prev_card != NULL) {
		card = prev_card;
		prev_card = card->prevCard;
	}
	return card;
}

String TR_Page::asString(void) {
	String s;

	s = "{'title':'"+name+"'";
	
	//write cards
	TR_Card* card = getFirstCard();
	if (card != NULL) {
		//write first card
		s += ",'cards':[";
		s += card->asString();

		//write additional cards
		TR_Card *next_card = card->nextCard;
		while (next_card != NULL) {
			card = next_card;
			s += ",";
			s += card->asString();        
			next_card = card->nextCard;
		}
	
		//finish the list
		s += "]"; 
	}
	
	//close out
	s += "}";

	return s;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////////////

TR_Page* TympanRemoteFormatter::addPage(String name) {
	
	//make a new page
	TR_Page *page = new TR_Page();
	if (page==NULL) return page; //return if we can't allocate it
	
	//link it to the previous page (linked list)
	TR_Page *prevLastPage = lastPage;
	if (prevLastPage != NULL) prevLastPage->nextPage = page;
	
	//assign the new page to be the new last page
	lastPage = page;
	nCustomPages++;
	
	//add the data
	page->name = name;
	page->prevPage = prevLastPage;
	
	//we're done!
	return page;
} 


void TympanRemoteFormatter::addPredefinedPage(String s) {
	if (predefPages.length()>0) {
		predefPages += ",";
	}
	predefPages += "'"+s+"'";
	nPredefinedPages++;
}


TR_Page* TympanRemoteFormatter::getFirstPage(void) {
	TR_Page *page = lastPage;
	if (page == NULL) return NULL;
	
	//keep stepping back to find the first
	TR_Page *prev_page = page->prevPage;
	while (prev_page != NULL) {
		page = prev_page;
		prev_page = page->prevPage;
	}
	return page;
}

/* int TympanRemoteFormatter::c_str(char *c_str, int maxLen) {
	int dest=0, source=0;
	String s;
	
	s = String(F("JSON={'icon':'tympan.png',"));
	source = 0;
	while ((dest < maxLen) && (source < (int)s.length())) c_str[dest++] = s[source++];
	
	// Add pages:
	if (nCustomPages > 0) {
		s = String("'pages':[") + pages[0].asString();
		source = 0;	while ((dest < maxLen) && (source < (int)s.length())) c_str[dest++] = s[source++];
	}
	for (int i=1; i<nCustomPages; i++) {
		s  = String(",") + pages[i].asString();
		source = 0;	while ((dest < maxLen) && (source < (int)s.length())) c_str[dest++] = s[source++];
	}
	if (nCustomPages > 0) {
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
} */


String TympanRemoteFormatter::asString(void)  {
	String s;

	s = F("JSON={'icon':'tympan.png',");

	// Add pages
	s += "'pages':[";
	//bool anyCustomPages = false;  //we don't know yet
	TR_Page* page = getFirstPage();
	if (page != NULL) {
		//anyCustomPages = true;
		
		//write the first page
		s += page->asString();


		//add subsequent pages
		TR_Page* next_page = page->nextPage;
		while (next_page != NULL) {

			page = next_page;
			s += ",";
			s += page->asString();        
			next_page = page->nextPage;
		} 
	}
	//close out the custom pages
	s += "]";     
	
	// Add predefined pages:
	if (predefPages.length()>1) {
		//if (s.length()>1) {
			s += ",";        
		//}
		
		s += F("'prescription':{'type':'BoysTown','pages':[") + predefPages +"]}";
	}
	
	//close out
	s += "}";

	return s;      
}
      
void TympanRemoteFormatter::asValidJSON(void) {
	String s;
	s = this->asString();
	s.replace("'","\"");
	Serial.println(F("Pretty printed (a.k.a. valid json):"));
	Serial.println(s);
}	  


void TympanRemoteFormatter::deleteAllPages(void) {
	//loop through and delete each page that was created
	TR_Page *page = lastPage;
	while (page != NULL) {
		TR_Page *prev_page = page->prevPage; //look to the next loopo
		delete page;  //delete the current page
		nCustomPages--; //decrement the count

		page = prev_page; //prepare for next loop
		if (page != NULL) page->nextPage = NULL; //there is no more next page
		lastPage = page; //update what the new lastPage is
	}
	
	//clear out the predefined pages
	predefPages = String("");
	nPredefinedPages = 0;
	
}