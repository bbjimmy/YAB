/*  URLView 2.11
	written by William Kakes of Tall Hill Software.
	
	This class provides an underlined and clickable BStringView
	that will launch the web browser, e-mail program, or FTP client
	when clicked on.  Other features include hover-highlighting,
	right-click	menus, and drag-and-drop support.

	You are free to use URLView in your own programs (both open-source
	and closed-source) free of charge, but a mention in your read me
	file or your program's about box would be appreciated.  See
	http://www.tallhill.com	for current contact information.
	
	URLView is provided as-is, with no warranties of any kind.  If
	you use it, you are on your own.
*/



#include "URLView.h"

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <fs_attr.h>
#include <MenuItem.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>
#include <Clipboard.h>

#include <unistd.h>

// #include <printf.h>
#include "global.h"

#ifndef ZETA
	#define _T(str) str
#else
	#include <locale/Locale.h>
#endif

URLView::URLView( BRect frame, const char *name, const char *label,
				  const char *url, uint32 resizingMode, uint32 flags )
		: BStringView( frame, name, label, resizingMode, flags ) {
	
	// Set the default values for the other definable instance variables.
	this->color = blue;
	this->clickColor = red;
	this->hoverColor = dark_blue;
	this->disabledColor = gray;
	this->hoverEnabled = true;
	this->draggable = true;
	this->iconSize = 16;
	this->underlineThickness = 1;
	
	// The link should be enabled by default (unless the URL is invalid, which
	// is handled by the SetURL() function).
	enabled = true;
	
	// Set the instance variables.
	this->url = 0;
	SetURL( url );
	
	// Create the cursor to use when over the link.
	this->linkCursor = new BCursor( url_cursor );
	
	// The link is not currently selected.
	selected = false;
	
	// The URL is currently not hover-colored.
	hovering = false;
	
	// The user has not dragged out of the view.
	draggedOut = false;
	
	// The user has not yet opened the popup menu.
	inPopup = false;
	
	// Initialize the attributes list (there are 14 standard
	// Person attributes).
	attributes = new BList( 14 );

}
		

URLView::~URLView() {
	delete url;
	delete linkCursor;
	
	// Delete all the attributes.
	KeyPair *item; 
	for( int i = 0;  (item = (KeyPair *) attributes->ItemAt(i));  i++ ) {
		delete item->key;
		delete item->value;
		delete item;
	}
	delete attributes;
}





void URLView::AttachedToWindow() {
	// When the view is first attached, we want to draw the link
	// in the normal color.  Also, we want to set our background color
	// to meet that of our parent.
	if( IsEnabled() )
		SetHighColor( color );
	else
		SetHighColor( disabledColor );
		
	if( Parent() != NULL ) {
		SetLowColor( Parent()->ViewColor() );
		SetViewColor( Parent()->ViewColor() );
	}
}



void URLView::Draw( BRect updateRect ) {
	BRect rect = Frame();
	rect.OffsetTo( B_ORIGIN );

	// We want 'g's, etc. to go below the underline.  When the BeOS can
	// do underlining of any font, this code can be removed.
	font_height height;
	GetFontHeight( &height );
	float descent = height.descent;
	
	// We want to be sensitive to the SetAlignment() function.
	float left, right;
	if( Alignment() == B_ALIGN_RIGHT ) {
		left = rect.right - StringWidth( Text() ) + 1;
		right = rect.right - 1;
	}
	else if( Alignment() == B_ALIGN_CENTER ) {
		left = rect.left + (rect.Width() / 2) - (StringWidth( Text() ) / 2);
		right = left + StringWidth( Text() ) - 2;
	}
	else {
		left = rect.left;
		right = left + StringWidth( Text() ) - 2;
	}

	// Draw the underline in the requested thickness.
	// Note:  If the link is disabled, we don't want to draw the underline.
	if( IsEnabled() ) {
		FillRect( BRect( (float) left,
						 (float) (rect.bottom - descent),
						 (float) right,
						 (float) (rect.bottom - descent + (underlineThickness - 1)) ) );
	}
	MovePenTo( BPoint( left, rect.bottom - descent - 1 ) );
			
	// Note:  DrawString() draws the text at one pixel above the pen's
	//		  current y coordinate.
	DrawString( Text() );
}



void URLView::MessageReceived( BMessage *message ) {
	// Is this a message from Tracker in response to our drag-and-drop?
	if( message->what == 'DDCP' ) {
		// Tracker will send back the name and path of the created file.
		// We need to read this information.
		entry_ref ref;
		message->FindRef( "directory", &ref );
		BEntry entry( &ref );
		BPath path( &entry );
		BString *fullName = new BString( path.Path() );
		fullName->Append( "/" );
		fullName->Append( message->FindString( "name" ) );
		BString *title = new BString( Text() );
		
		// Set the new file as a bookmark or as a person as appropriate.
		if( IsEmailLink() ) {
			CreatePerson( fullName, title );
		}
		else CreateBookmark( fullName, title );
		
		delete fullName;
		delete title;
	}
}



void URLView::MouseDown( BPoint point ) {
	// If the link isn't enabled, don't do anything.
	if( !IsEnabled() )
		return;

	// See which mouse buttons were clicked.
	int32 buttons = Window()->CurrentMessage()->FindInt32( "buttons" );

	// We want to highlight the text if the user clicks on
	// the URL.  We want to be sure to only register a click
	// if the user clicks on the link text itself and not just
	// anywhere in the view.
	if( GetTextRect().Contains( point ) ) {
		SetHighColor( clickColor );
		Redraw();
		
		// Set the link as selected and track the mouse.
		selected = true;
		SetMouseEventMask( B_POINTER_EVENTS );
		
		// Remember where the user clicked so we can correctly
		// offset the transparent URL if the user drags.
		BRect frame = Frame();
		frame.OffsetTo( B_ORIGIN );
		dragOffset = point;
		if( Alignment() == B_ALIGN_RIGHT ) {
			dragOffset.x -= frame.Width() - StringWidth( Text() );
		}
		else if( Alignment() == B_ALIGN_CENTER ) {
			dragOffset.x -= (frame.Width() / 2) - (StringWidth( Text() ) / 2);
		}
		
		// Pop up the context menu?
		if( buttons == B_SECONDARY_MOUSE_BUTTON ) inPopup = true;
	}
}



void URLView::MouseMoved( BPoint point, uint32 transit,
						  const BMessage *message ) {
						  
	// If the link isn't enabled, don't do anything.				  
	if( !IsEnabled() )
		return;
						  
	// Make sure the window is the active one.
	if( !Window()->IsActive() ) return;

	// See which mouse buttons were clicked.
	int32 buttons = Window()->CurrentMessage()->FindInt32( "buttons" );
	// Is the user currently dragging the link?  (i.e. is a mouse button
	// currently down?)
	bool alreadyDragging = (buttons != 0);

	switch( transit ) {
		case( B_ENTERED_VIEW ):
			// Should we set the cursor to the link cursor?
			if( GetTextRect().Contains( point )  &&  !draggedOut ) {
				if( !alreadyDragging ) be_app->SetCursor( linkCursor );
				
				// Did the user leave and re-enter the view while
				// holding down the mouse button?  If so, highlight
				// the link.
				if( selected ) {
					SetHighColor( clickColor );
					Redraw();
				}
				// Should we hover-highlight the link?
				else if( hoverEnabled  &&  !alreadyDragging ) {
					if( buttons == 0 ) {
						SetHighColor( hoverColor );
						Redraw();
						hovering = true;
					}
				}	
			}
			break;
			
		case( B_EXITED_VIEW ):
			// We want to restore the link to it normal color and the
			// mouse cursor to the normal hand.  However, we should only
			// set the color and re-draw if it is needed.
			if( selected  &&  !draggedOut ) {
				be_app->SetCursor( B_HAND_CURSOR );
				SetHighColor( color );
				Redraw();
				
				// Is the user drag-and-dropping a bookmark or person?
				if( draggable ) {
					draggedOut = true;
					if( IsEmailLink() )	DoPersonDrag();
					else DoBookmarkDrag();
				}
			}
			// Is the link currently hover-highlighted?  If so, restore
			// the normal color now.
			else if( hovering  &&  !alreadyDragging ) {
				be_app->SetCursor( B_HAND_CURSOR );
				SetHighColor( color );
				Redraw();
				hovering = false;
			}
			// Change the cursor back to the hand.
			else {
				be_app->SetCursor( B_HAND_CURSOR );
			}
			break;

		case( B_INSIDE_VIEW ):
			// The user could either be moving out of the view or
			// back into it here, so we must handle both cases.
			// In the first case, the cursor is now over the link.
			if( GetTextRect().Contains( point )  &&  !draggedOut ) {
				// We only want to change the cursor if not dragging.						
				if( !alreadyDragging ) be_app->SetCursor( linkCursor );
				if( selected ) {
					if( draggable ) {
						// If the user moves the mouse more than ten
						// pixels, begin the drag.
						if( (point.x - dragOffset.x) > 10  ||
							(dragOffset.x - point.x) > 10  ||
							(point.y - dragOffset.y) > 10  ||
							(dragOffset.y - point.y) > 10 ) {
							draggedOut = true;
							// Draw the appropriate drag object, etc.
							if( IsEmailLink() ) DoPersonDrag();
							else DoBookmarkDrag();
							SetHighColor( color );
							Redraw();
						}
					}
					else {
						// Since the link is not draggable, highlight it
						// as long as the user holds the button down and
						// has the mouse cursor over it (like a standard
						// button).
						SetHighColor( clickColor );
						Redraw();
					}
				}
				// The link isn't currently selected?  If hover-highlighting
				// is enabled, highlight the link.
				else if( hoverEnabled  && !alreadyDragging ) {
					SetHighColor( hoverColor );
					Redraw();
					hovering = true;
				}
			}
			// In this case, the mouse cursor is not over the link, so we
			// need to restore the original link color, etc.
			else if( !draggedOut ) {
				be_app->SetCursor( B_HAND_CURSOR );
				if( selected ) {
					SetHighColor( color );
					Redraw();
					
					// Is the user dragging the link?
					if( draggable ) {
						draggedOut = true;
						if( IsEmailLink() ) DoPersonDrag();
						else DoBookmarkDrag();
					}
				}
				// Is the mouse cursor hovering over the link?
				else if( hovering ) {
					SetHighColor( color );
					Redraw();
					hovering = false;
				}
			}
			break;
	}
}



void URLView::MouseUp( BPoint point ) {
	// If the link isn't enabled, don't do anything.
	if( !IsEnabled() )
		return;

	// Do we want to show the right-click menu?
	if( inPopup  &&  GetTextRect().Contains( point ) ) {
		BPopUpMenu *popup = CreatePopupMenu();
			// Work around a current bug in Be's popup menus.
			point.y = point.y - 6;
			
			// Display the popup menu.
			BMenuItem *selected = popup->Go( ConvertToScreen( point ) , false, true );
			
			// Did the user select an item?
			if( selected ) {
				BString label( selected->Label() );
				// Did the user select the first item?  If so, launch the URL.
				if( label.FindFirst( "Open" ) != B_ERROR  ||
					label.FindFirst( "Send" ) != B_ERROR  ||
					label.FindFirst( "Connect" ) != B_ERROR ) {
					LaunchURL();
				}
				// Did the user select the second item?
				else if( label.FindFirst( "Copy" ) != B_ERROR ) {
					CopyToClipboard();
				}
			}
			// If not, restore the normal link color.
			else {
				SetHighColor( color );
				Redraw();
			}
		}

	// If the link was clicked on (and not dragged), run the program
	// that should handle the URL.
	if( selected  &&  GetTextRect().Contains( point )  &&
		!draggedOut  &&  !inPopup ) {
		LaunchURL();
	}
	selected = false;
	draggedOut = false;
	inPopup = false;
	
	// Should we restore the hovering-highlighted color or the original
	// link color?
	if( GetTextRect().Contains( point )  &&  !draggedOut  &&
		!inPopup  &&  hoverEnabled ) {
		SetHighColor( hoverColor );
	}
	else if( !hovering ) SetHighColor( color );
	Redraw();
}



void URLView::WindowActivated( bool active ) {
	// Be sure that if the user clicks on a link prompting the opening of
	// a new window (i.e. launching NetPositive) the URL is not left drawn
	// with the hover color.
	if( !active ) {
		if( IsEnabled() ) {
			SetHighColor( color );
			Redraw();
		}
	}
}





void URLView::AddAttribute( const char *name, const char *value ) {
	// Add an attribute (name and corresponding value) to the object
	// that will be dragged out (i.e. to fill in Person fields, etc.)
	KeyPair *newPair = new KeyPair;
	newPair->key = new BString( name );
	newPair->value = new BString( value );
	attributes->AddItem( newPair );
}



bool URLView::IsEnabled() {
	// Return whether or not this link is enabled (and therefore clickable).
	return enabled;
}




void URLView::SetColor( rgb_color color ) {
	// Set the normal link color.
	this->color = color;
	if( IsEnabled() ) {
		Window()->Lock();
		SetHighColor( color );
		Redraw();
		Window()->Unlock();
	}
}


void URLView::SetColor( uchar red, uchar green, uchar blue, uchar alpha ) {
	// Set the normal link color.
	rgb_color color;
	color.red	= red;
	color.green	= green;
	color.blue	= blue;
	color.alpha	= alpha;
	SetColor( color );
}


void URLView::SetClickColor( rgb_color color ) {
	// Set the link color used when the link is clicked.
	clickColor = color;
}


void URLView::SetClickColor( uchar red, uchar green, uchar blue, uchar alpha ) {
	// Set the link color used when the link is clicked.
	rgb_color color;
	color.red	= red;
	color.green	= green;
	color.blue	= blue;
	color.alpha	= alpha;
	SetClickColor( color );
}


void URLView::SetDisabledColor( rgb_color color ) {
	// Set the color to draw in when the link is disabled.
	disabledColor = color;
	Window()->Lock();
	Redraw();
	Window()->Unlock();
}


void URLView::SetDisabledColor( uchar red, uchar green, uchar blue, uchar alpha ) {
	// Set the color to draw in when the link is disabled.
	rgb_color color;
	color.red	= red;
	color.green	= green;
	color.blue	= blue;
	color.alpha	= alpha;
	SetDisabledColor( color );
}


void URLView::SetDraggable( bool draggable ) {
	// Set whether or not this link is draggable.
	this->draggable = draggable;
}


void URLView::SetEnabled( bool enabled ) {
	// Set whether or not the link is enabled (and therefore clickable).
	bool redraw = this->enabled != enabled;
	this->enabled = enabled;
	
	if( Window() ) {
		Window()->Lock();
		if( !enabled )
			SetHighColor( disabledColor );
		else
			SetHighColor( color );
		if( redraw )
			Invalidate();
		Window()->Unlock();
	}
}


void URLView::SetHoverColor( rgb_color color ) {
	// Set the link color used when the mouse cursor is over it.
	hoverColor = color;
}


void URLView::SetHoverColor( uchar red, uchar green, uchar blue, uchar alpha ) {
	// Set the color to draw in when the link is disabled.
	rgb_color color;
	color.red	= red;
	color.green	= green;
	color.blue	= blue;
	color.alpha	= alpha;
	SetHoverColor( color );
}


void URLView::SetHoverEnabled( bool hover ) {
	// Set whether or not to hover-highlight the link.
	hoverEnabled = hover;
}


void URLView::SetIconSize( icon_size iconSize ) {
	// Set the size of the icon that will be shown when the link is dragged.
	if( iconSize == B_MINI_ICON ) this->iconSize = 16;
	else this->iconSize = 32;
}


void URLView::SetUnderlineThickness( int thickness ) {
	// Set the thickness of the underline in pixels.
	underlineThickness = thickness;
}


void URLView::SetURL( const char *url ) {
	// Set the URL value.
	delete this->url;
	this->url = new BString( url );

	// If it's an e-mail link, we want to insert "mailto:" to the front
	// if the user did not enter it.
	if( IsEmailLink() ) {
		if( this->url->FindFirst( "mailto:" ) != 0 ) {
			this->url->Prepend( "mailto:" );
			return;
		}
	}
	
	// We want to see if the URL is valid.  If not, we will disable it.
	if( !IsFTPLink()  &&  !IsHTMLLink() )
		SetEnabled( false );
}





void URLView::CopyToClipboard() {
	// Copy the URL to the clipboard.
	BClipboard clipboard( "system" );
	BMessage *clip = (BMessage *) NULL;
	// Get the important URL (i.e. trim off "mailto:", etc.).
	BString newclip = GetImportantURL();
	
	// Be sure to lock the clipboard first.
	if( clipboard.Lock() ) {
		clipboard.Clear();
		if( (clip = clipboard.Data()) ) {
			clip->AddData( "text/plain", B_MIME_TYPE, newclip.String(),
						   newclip.Length() + 1 );
			clipboard.Commit();
		}
		clipboard.Unlock();
	}
}



void URLView::CreateBookmark( const BString *fullName, const BString *title ) {
	// Read the file defined by the path and the title.
	BFile *file = new BFile( fullName->String(), B_WRITE_ONLY );

	// Set the file's MIME type to be a bookmark.
	BNodeInfo *nodeInfo = new BNodeInfo( file );
	nodeInfo->SetType( "application/x-vnd.Be-bookmark" );
	delete nodeInfo;
	delete file;

	// Add all the attributes, both those inherrent to bookmarks and any
	// the developer may have defined using AddAttribute().
	DIR *d;
	int fd;
	d = fs_open_attr_dir( fullName->String() );
	if( d ) {
		fd = open( fullName->String(), O_WRONLY );
		fs_write_attr( fd, "META:title", B_STRING_TYPE, 0, title->String(), title->Length() + 1 );
		fs_write_attr( fd, "META:url", B_STRING_TYPE, 0, url->String(), url->Length() + 1 );
		WriteAttributes( fd );
		close( fd );
		fs_close_attr_dir( d ); 
	}
}


void URLView::CreatePerson( const BString *fullName, const BString *title ) {
	// Read the file defined by the path and the title.
	BFile *file = new BFile( fullName->String(), B_WRITE_ONLY );
		
	// Set the file's MIME type to be a person.
	BNodeInfo *nodeInfo = new BNodeInfo( file );
	nodeInfo->SetType( "application/x-person" );
	delete nodeInfo;
	delete file;

	// Add all the attributes, both those inherrent to person files and any
	// the developer may have defined using AddAttribute().
	DIR *d;
	int fd;
	d = fs_open_attr_dir( fullName->String() );
	if( d ) {
		fd = open( fullName->String(), O_WRONLY );
		fs_write_attr( fd, "META:name", B_STRING_TYPE, 0, title->String(), title->Length() + 1 );
		BString email = GetImportantURL();
		fs_write_attr( fd, "META:email", B_STRING_TYPE, 0, email.String(), email.Length() + 1 );
		WriteAttributes( fd );
		close( fd );
		fs_close_attr_dir( d ); 
	}
}



BPopUpMenu * URLView::CreatePopupMenu() {
	// Create the right-click popup menu.
	BPopUpMenu *returnMe = new BPopUpMenu( "URLView Popup", false, false );
	returnMe->SetAsyncAutoDestruct( true );
	
	entry_ref app;

	// Set the text of the first item according to the link type.	
	if( IsEmailLink() ) {
		// Find the name of the default e-mail client.
		if( be_roster->FindApp( "text/x-email", &app ) == B_OK ) {
			BEntry entry( &app );
			BString openLabel( _T("Send e-mail to this address using ") );
			char name[B_FILE_NAME_LENGTH];
			entry.GetName( name );
			openLabel.Append( name );
			returnMe->AddItem( new BMenuItem( openLabel.String(), NULL ) );
		}
	}
	else if( IsFTPLink() ) {
		// Find the name of the default FTP client.
		if( be_roster->FindApp( "application/x-vnd.Be.URL.ftp", &app ) == B_OK ) {
			BEntry entry( &app );
			BString openLabel( _T("Connect to this server using ") );
			char name[B_FILE_NAME_LENGTH];
			entry.GetName( name );
			openLabel.Append( name );
			returnMe->AddItem( new BMenuItem( openLabel.String(), NULL ) );
		}
	}
	else {
		// Find the name of the default HTML handler (browser).
		if( be_roster->FindApp( "text/html", &app ) == B_OK ) {
			BEntry entry( &app );
			BString openLabel( _T("Open this link using ") );
			char name[B_FILE_NAME_LENGTH];
			entry.GetName( name );
			openLabel.Append( name );
			returnMe->AddItem( new BMenuItem( openLabel.String(), NULL ) );
		}
	}
	returnMe->AddItem( new BMenuItem( _T("Copy this link to the clipboard"), NULL ) );
	
	return returnMe;
}



void URLView::DoBookmarkDrag() {
	// Handle all of the bookmark dragging.  This includes setting up
	// the drag message and drawing the dragged bitmap.
	
	// Set up the drag message to support both BTextView dragging (using
	// the URL) and file dropping (to Tracker).
	BMessage *dragMessage = new BMessage( B_MIME_DATA );
	dragMessage->AddInt32( "be:actions", B_COPY_TARGET );
	dragMessage->AddString( "be:types", "application/octet-stream" );
	dragMessage->AddString( "be:filetypes", "application/x-vnd.Be-bookmark" );
	dragMessage->AddString( "be:type_descriptions", "bookmark" );
	dragMessage->AddString( "be:clip_name", Text() );
	dragMessage->AddString( "be:url", url->String() );
	
	// This allows the user to drag the URL into a standard BTextView.
	BString link = GetImportantURL();
	dragMessage->AddData( "text/plain", B_MIME_DATA, link.String(),
						  link.Length() + 1 );
			
	// Query for the system's icon for bookmarks.
	BBitmap *bookmarkIcon = new BBitmap( BRect( 0, 0, iconSize - 1,
												iconSize - 1 ), B_CMAP8 );
	BMimeType mime( "application/x-vnd.Be-bookmark" );
	if( iconSize == 16 ) mime.GetIcon( bookmarkIcon, B_MINI_ICON );
	else mime.GetIcon( bookmarkIcon, B_LARGE_ICON );
	
	// Find the size of the bitmap to drag.  If the text is bigger than the
	// icon, use that size.  Otherwise, use the icon's.  Center the icon
	// vertically in the bitmap.
	BRect urlRect = GetURLRect();
	BRect rect = urlRect;
	rect.right += iconSize + 4;
	if( (rect.bottom - rect.top) < iconSize ) {
		int adjustment = (int) ((iconSize - (rect.bottom - rect.top)) / 2) + 1;
		rect.top -= adjustment;
		rect.bottom += adjustment;
	}
	
	// Make sure the rectangle starts at 0,0.
	rect.bottom += 0 - rect.top;
	rect.top = 0;
	
	// Create the bitmap to draw the dragged image in.
	BBitmap *dragBitmap = new BBitmap( rect, B_RGBA32, true );
	BView *dragView = new BView( rect, "Drag View", 0, 0 );
	dragBitmap->Lock();
	dragBitmap->AddChild( dragView );
	
	BRect frameRect = dragView->Frame();
	
	// Make the background of the dragged image transparent.
	dragView->SetHighColor( B_TRANSPARENT_COLOR );
	dragView->FillRect( frameRect );

	// We want 'g's, etc. to go below the underline.  When the BeOS can
	// do underlining of any font, this code can be removed.
	font_height height;
	GetFontHeight( &height );
	float descent = height.descent;

	// Find the vertical center of the view so we can vertically
	// center everything.
	int centerPixel = (int) ((frameRect.bottom - frameRect.top) / 2);
	int textCenter  = (int) (descent + underlineThickness) + centerPixel;

	// We want to draw everything only half opaque.
	dragView->SetDrawingMode( B_OP_ALPHA );
	dragView->SetHighColor( color.red, color.green, color.blue, 128.0 );
	dragView->SetBlendingMode( B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE );

	// Center the icon in the view.
	dragView->MovePenTo( BPoint( frameRect.left,
								 centerPixel - (iconSize / 2) ) );
	dragView->DrawBitmap( bookmarkIcon );

	// Draw the text in the same font (size, etc.) as the link view.
	// Note:  DrawString() draws the text at one pixel above the pen's
	//		  current y coordinate.
	BFont font;
	GetFont( &font );
	dragView->SetFont( &font );
	dragView->MovePenTo( BPoint( frameRect.left + iconSize + 4, textCenter ) );
	dragView->DrawString( url->String() );

	// Draw the underline in the requested thickness.
	dragView->FillRect( BRect( (float) frameRect.left + iconSize + 4,
						(float) (textCenter + 1),
						(float) StringWidth( url->String() ) + iconSize + 4,
						(float) textCenter + underlineThickness ) );
	
	// Be sure to flush the view buffer so everything is drawn.
	dragView->Flush();
	dragBitmap->Unlock();
	
	// The URL's label is probably not the same size as the URL's
	// address, which is what we're going to draw.  So horizontally
	// offset the bitmap proportionally to where the user clicked
	// on the link.
	float horiz = dragOffset.x / GetTextRect().Width();
	dragOffset.x = horiz * frameRect.right;
	
	DragMessage( dragMessage, dragBitmap, B_OP_ALPHA,
				 BPoint( dragOffset.x, (rect.Height() / 2) + 2 ), this );
	delete dragMessage;

	draggedOut = true;
}


void URLView::DoPersonDrag() {
	// Handle all of the bookmark dragging.  This includes setting up
	// the drag message and drawing the dragged bitmap.
	
	// Set up the drag message to support both BTextView dragging (using
	// the e-mail address) and file dropping (to Tracker).
	BMessage *dragMessage = new BMessage( B_MIME_DATA );
	dragMessage->AddInt32( "be:actions", B_COPY_TARGET );
	dragMessage->AddString( "be:types", "application/octet-stream" );
	dragMessage->AddString( "be:filetypes", "application/x-person" );
	dragMessage->AddString( "be:type_descriptions", "person" );
	dragMessage->AddString( "be:clip_name", Text() );
	
	// This allows the user to drag the e-mail address into a
	// standard BTextView.
	BString email = GetImportantURL();
	dragMessage->AddData( "text/plain", B_MIME_DATA, email.String(),
						  email.Length() + 1 );
	
	// Query for the system's icon for bookmarks.
	BBitmap *personIcon = new BBitmap( BRect( 0, 0, iconSize - 1,
									   iconSize - 1 ), B_CMAP8 );
	#ifdef ZETA
		BMimeType mime( "application/x-vnd.Be-PEPL" );
	#else
		BMimeType mime( "application/x-person" );
	#endif
	if( iconSize == 16 ) mime.GetIcon( personIcon, B_MINI_ICON );
	else mime.GetIcon( personIcon, B_LARGE_ICON );
	
	// Find the size of the bitmap to drag.  If the text is bigger than the
	// icon, use that size.  Otherwise, use the icon's.  Center the icon
	// vertically in the bitmap.
	BRect rect = GetTextRect();
	rect.right += iconSize + 4;
	if( (rect.bottom - rect.top) < iconSize ) {
		int adjustment = (int) ((iconSize - (rect.bottom - rect.top)) / 2) + 1;
		rect.top -= adjustment;
		rect.bottom += adjustment;
	}
	
	// Make sure the rectangle starts at 0,0.
	rect.bottom += 0 - rect.top;
	rect.top = 0;
	
	// Create the bitmap to draw the dragged image in.
	BBitmap *dragBitmap = new BBitmap( rect, B_RGBA32, true );
	BView *dragView = new BView( rect, "Drag View", 0, 0 );
	dragBitmap->Lock();
	dragBitmap->AddChild( dragView );
	
	BRect frameRect = dragView->Frame();
	
	// Make the background of the dragged image transparent.
	dragView->SetHighColor( B_TRANSPARENT_COLOR );
	dragView->FillRect( frameRect );

	// We want 'g's, etc. to go below the underline.  When the BeOS can
	// do underlining of any font, this code can be removed.
	font_height height;
	GetFontHeight( &height );
	float descent = height.descent;

	// Find the vertical center of the view so we can vertically
	// center everything.
	int centerPixel = (int) ((frameRect.bottom - frameRect.top) / 2);
	int textCenter  = (int) (descent + underlineThickness) + centerPixel;

	// We want to draw everything only half opaque.
	dragView->SetDrawingMode( B_OP_ALPHA );
	dragView->SetHighColor( 0.0, 0.0, 0.0, 128.0 );
	dragView->SetBlendingMode( B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE );

	// Center the icon in the view.
	dragView->MovePenTo( BPoint( frameRect.left,
								 centerPixel - (iconSize / 2) ) );
	dragView->DrawBitmap( personIcon );

	// Draw the text in the same font (size, etc.) as the link view.
	// Note:  DrawString() draws the text at one pixel above the pen's
	//		  current y coordinate.
	BFont font;
	GetFont( &font );
	dragView->SetFont( &font );
	dragView->MovePenTo( BPoint( frameRect.left + iconSize + 4, textCenter ) );
	dragView->DrawString( Text() );
	
	// Be sure to flush the view buffer so everything is drawn.
	dragView->Flush();
	dragBitmap->Unlock();
	
	// The Person icon adds some width to the bitmap that we are
	// going to draw.  So horizontally offset the bitmap proportionally
	// to where the user clicked on the link.
	float horiz = dragOffset.x / GetTextRect().Width();
	dragOffset.x = horiz * frameRect.right;

	DragMessage( dragMessage, dragBitmap, B_OP_ALPHA,
				 BPoint( dragOffset.x,
				 		 (rect.Height() + underlineThickness) / 2 + 2), this );
	delete dragMessage;

	draggedOut = true;
}



BString URLView::GetImportantURL() {
	// Return the relevant portion of the URL (i.e. strip off "mailto:" from
	// e-mail address URLs).
	BString returnMe;
	
	if( IsEmailLink() ) url->CopyInto( returnMe, 7, url->CountChars() - 6 );
	else url->CopyInto( returnMe, 0, url->CountChars() );

	return returnMe;
}



BRect URLView::GetTextRect() {
	// This function will return a BRect that contains only the text
	// and the underline, so the mouse can change and the link will
	// be activated only when the mouse is over the text itself, not
	// just within the view.
	
	// Note:  We'll use bounding boxes, because they are the most
	//        accurate, and since the user is interacting with the
	//        view, off-by-one-pixel errors look bad.
	const char *textArray[1];
	textArray[0] = Text();
	
	escapement_delta delta;
	delta.nonspace = 0;
	delta.space = 0;
	escapement_delta escapements[1];
	escapements[0] = delta;
	
	BRect returnMe;
	BRect rectArray[1];
	rectArray[0] = returnMe;
	
	BFont font;
	GetFont( &font );
	font.GetBoundingBoxesForStrings( textArray, 1, B_SCREEN_METRIC, escapements, rectArray );

	BRect frame = Frame();
	frame.OffsetTo( B_ORIGIN );
	returnMe = rectArray[0];
	
	// Get the height of the current font.
	font_height height;
	GetFontHeight( &height );
	float descent = height.descent;
	
	// Account for rounding of the floats when drawn to avoid
	// one-pixel-off errors.
	float lowerBound = 0;
	if( (((int) descent) * 2) != ((int) (descent * 2)) )
		lowerBound = 1;
	
	// Adjust the bounding box to reflect where the text is in the view.
	returnMe.bottom += 1;
	returnMe.OffsetTo( B_ORIGIN );
	float rectHeight = returnMe.Height();
	returnMe.bottom = frame.bottom - descent;
	returnMe.top = returnMe.bottom - rectHeight;
	returnMe.bottom += 1 + underlineThickness;
	returnMe.OffsetBy( 0.0, -(1 + lowerBound) );

	return returnMe;
}


BRect URLView::GetURLRect() {
	// This function will return a BRect that contains only the URL
	// and the underline, so we can draw it when the user drags.
	// We'll use GetFontHeight() instead of bounding boxes here
	// because a possible pixel or two off doesn't matter, whereas it
	// does when detecting if the user has the cursor over the link.
	BRect frame = Frame();
	frame.OffsetTo( B_ORIGIN );

	// Get the height of the current font.
	font_height height;
	GetFontHeight( &height );
	
	float stringHeight = underlineThickness + height.ascent - 1;
	
	// Get the rectangle of just the string.
	return BRect( frame.left, frame.bottom - stringHeight,
				  frame.left + StringWidth( url->String() ),
				  frame.bottom - 1 );
}


bool URLView::IsEmailLink() {
	// Is this link an e-mail link?
	if( url->FindFirst( "mailto:" ) == 0 )
		return true;
	
	if( !IsHTMLLink()  &&  !IsFTPLink()  &&
		url->FindFirst( "@" ) != B_ERROR ) {
		return true;
	}

	return false;
}


bool URLView::IsFTPLink() {
	// Is this link an FTP link?
	return( url->FindFirst( "ftp://" ) == 0 );
}


bool URLView::IsHTMLLink() {
	// Is this link an HTML or file link?
	return( (url->FindFirst( "http://" ) == 0 )  ||
			(url->FindFirst( "file://" ) == 0 ) );
}



void URLView::LaunchURL() {
	// Is the URL a mail link or HTTP?
	if( IsEmailLink() ) {
		// Lock the string buffer and pass it to the mail program.
		char *link = url->LockBuffer( url->Length()+1 );
		status_t result = be_roster->Launch( "text/x-email", 1, &link );
		url->UnlockBuffer();
		// Make sure the user has an e-mail program.
		if( result != B_NO_ERROR  &&  result != B_ALREADY_RUNNING ) {
			BAlert *alert = new BAlert( "E-mail Warning",
										"There is no e-mail program on your machine that is configured as the default program to send e-mail.",
										"Ok", NULL, NULL, B_WIDTH_AS_USUAL,
										B_WARNING_ALERT );
			alert->Go();
		}
	}
	// Handle an HTTP link.
	else if( IsHTMLLink() ) {
		// Lock the string buffer and pass it to the web browser.
		char *link = url->LockBuffer( url->Length()+1 );
		status_t result = be_roster->Launch( "text/html", 1, &link );
		url->UnlockBuffer();
		// Make sure the user has a web browser.
		if( result != B_NO_ERROR  &&  result != B_ALREADY_RUNNING ) {
			BAlert *alert = new BAlert( "Web Browser Warning",
										"There is no web browser on your machine that is configured as the default program to view web pages.",
										"Ok", NULL, NULL, B_WIDTH_AS_USUAL,
										B_WARNING_ALERT );
			alert->Go();
		}
	}
	// Handle an FTP link.
	else if( IsFTPLink() ) {
		// Lock the string buffer and pass it to the FTP client.
		char *link = url->LockBuffer( url->Length()+1 );
		status_t result = be_roster->Launch( "application/x-vnd.Be.URL.ftp",
											 1, &link );
		url->UnlockBuffer();
		// Make sure the user has an FTP client.
		if( result != B_NO_ERROR  &&  result != B_ALREADY_RUNNING ) {
			BAlert *alert = new BAlert( "FTP Warning",
										"There is no FTP client on your machine that is configured as the default program to connect to an FTP server.",
										"Ok", NULL, NULL, B_WIDTH_AS_USUAL,
										B_WARNING_ALERT );
			alert->Go();
		}
	}
	
	// We don't know how to handle anything else.
}



void URLView::Redraw() {
	// Redraw the link without flicker.
	BRect frame = Frame();
	frame.OffsetTo( B_ORIGIN );
	Draw( frame );
}


void URLView::WriteAttributes( int fd ) {
	// Write the developer-defined attributes to the newly-created file.
	KeyPair *item; 
	for( int i = 0;  (item = (KeyPair *) attributes->ItemAt(i));  i++ ) {
		fs_write_attr( fd, item->key->String(), B_STRING_TYPE, 0, item->value->String(), item->value->Length() + 1 );
	}	
}
