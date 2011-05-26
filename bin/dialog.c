/*
 *  dialog.c:		Info or warning dialog windows	
 * 
 *  Written by:		Ullrich Hafner
 *		
 *  This file is part of FIASCO ([F]ractal [I]mage [A]nd [S]equence [CO]dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/07/04 16:27:43 $
 *  $Author: hafner $
 *  $Revision: 4.1 $
 *  $State: Exp $
 */

#include "config.h"

#include <gtk/gtk.h>

#include "types.h"
#include "macros.h"
#include "error.h"

#include "dialog.h"

/******************************************************************************

			     local variables
  
*****************************************************************************/

static char *info_xpm [] = {
"48 48 47 1",
" 	c None",
".	c #69A675D679E7",
"X	c #59655D7579E7",
"o	c #410349246185",
"O	c #69A671C69658",
"+	c #1861186130C2",
"@	c #1040104028A2",
"#	c #186120814103",
"$	c #208128A25965",
"%	c #208120814924",
"&	c #208128A24924",
"*	c #49244D346185",
"=	c #1040186130C2",
"-	c #30C238E371C6",
";	c #38E341038E38",
":	c #38E349248E38",
">	c #38E345148617",
",	c #38E33CF35965",
"<	c #5144555569A6",
"1	c #28A234D34924",
"2	c #30C2410379E7",
"3	c #51445D759658",
"4	c #D75CD75CDF7D",
"5	c #EFBEEBADEFBE",
"6	c #DF7DDB6CDF7D",
"7	c #596561859E79",
"8	c #410349248E38",
"9	c #28A230C26185",
"0	c #E79DE38DE79D",
"q	c #C71BC30BC71B",
"w	c #96589248A699",
"e	c #C71BCB2BCF3C",
"r	c #1861208138E3",
"t	c #410351448E38",
"y	c #79E77DF7A699",
"u	c #CF3CCB2BDF7D",
"i	c #861786179658",
"p	c #1040186128A2",
"a	c #410351449658",
"s	c #492451449658",
"d	c #F7DEF3CEF7DE",
"f	c #86178A28AEBA",
"g	c #D75CD34CD75C",
"h	c #492459659658",
"j	c #492459658E38",
"k	c #FFFFFBEEFFFF",
"l	c #1861186128A2",
"                                                ",
"                                                ",
"                                                ",
"                                                ",
"                                                ",
"                   .XXooooXOO                   ",
"                 ++@++#$$$$%%&**                ",
"               ==$$-;;;;::::::>>,<              ",
"             ,===$$-;;;;::::::>>,<<             ",
"           11@$$2;;;3345667:::888>>X            ",
"          #==9;;;;;;440qqqw228888888XX          ",
"          #==9;;;;;;440qqqw228888888XX          ",
"        11=--;;;;;;;55qqqqerr8888888tty         ",
"       <@@9;;;;;;;;:uuqqeeipp888888tttt         ",
"       <@@9;;;;;;;;:uuqqeeipp888888tttt         ",
"       +%%;;;;;;;:::77weiirrr8888ttttttOO       ",
"      <@--;;;;;;::::::2rppr22888ttttttass       ",
"      <@--;;;;;;::::::2rppr22888ttttttass       ",
"     11%;;;;;;::75555555ddd888ttttttaasssf      ",
"     ==9;;;;;:::7>>@eeeggggppttttttasssss3      ",
"     ==9;;;;;:::7>>@eeeggggppttttttasssss3      ",
"     @@2;;;::::::>>855gggg6ppttttaasssssss      ",
"     ==;;;:::::::888ddgg666pptttasssssssss      ",
"     ##;::::::::8888ddg6666pptaassssssssss      ",
"     ##;::::::::8888ddg6666pptaassssssssss      ",
"     ##:::::::888888dd66660ppasssssssssssh      ",
"     ++::::::8888888dd66000ppsssssssssshhh      ",
"     ++::::::8888888dd66000ppsssssssssshhh      ",
"     ##::::888888888dd60000ppssssssssshhhy      ",
"     <<2::888888888tdd00000ppssssssshhhhh       ",
"     <<2::888888888tdd00000ppssssssshhhhh       ",
"       &888888888tttdd00005ppsssssshhhhhh       ",
"       *88888888ttttdd00555ppsssshhhhhh         ",
"       *88888888ttttdd00555ppsssshhhhhh         ",
"        --8888ttttttdd55555ppssshhhhhh7         ",
"        iij88ttttddddd55dddddkhhhhhh77          ",
"         ij88ttttddddd55dddddkhhhhhh77          ",
"          Ojjttttaapppppppplllllhhh7            ",
"             3ttasssssssssshhhhhhOO             ",
"              ttasssssssssshhhhhhOO             ",
"                3ssssssssshhhh77                ",
"                    yyysyyf                     ",
"                                                ",
"                                                ",
"                                                ",
"                                                ",
"                                                ",
"                                                "};

static char *warning_xpm [] = {
"64 64 4 1",
"  	s None c None",
".	c #000000000000",
"X	c #FBEE10404103",
"o	c #FFFFFFFFFFFF",
"                                                                ",
"                                                                ",
"                                                                ",
"                               .                                ",
"                              .X.                               ",
"                              .X.                               ",
"                             .XXX.                              ",
"                             .XXX.                              ",
"                            .XXXXX.                             ",
"                            .XXXXX.                             ",
"                           .XXXXXXX.                            ",
"                           .XXXXXXX.                            ",
"                          .XXXXXXXXX.                           ",
"                          .XXXXXXXXX.                           ",
"                         .XXXXXXXXXXX.                          ",
"                         .XXXXX.XXXXX.                          ",
"                        .XXXXX.o.XXXXX.                         ",
"                        .XXXXX.o.XXXXX.                         ",
"                       .XXXXX.ooo.XXXXX.                        ",
"                       .XXXXX.ooo.XXXXX.                        ",
"                      .XXXXX.ooooo.XXXXX.                       ",
"                      .XXXXX.ooooo.XXXXX.                       ",
"                     .XXXXX.ooooooo.XXXXX.                      ",
"                     .XXXXX.ooooooo.XXXXX.                      ",
"                    .XXXXX.ooooooooo.XXXXX.                     ",
"                    .XXXXX.ooooooooo.XXXXX.                     ",
"                   .XXXXX.oooo...oooo.XXXXX.                    ",
"                   .XXXXX.ooo.....ooo.XXXXX.                    ",
"                  .XXXXX.oooo.....oooo.XXXXX.                   ",
"                  .XXXXX.oooo.....oooo.XXXXX.                   ",
"                 .XXXXX.ooooo.....ooooo.XXXXX.                  ",
"                 .XXXXX.ooooo.....ooooo.XXXXX.                  ",
"                .XXXXX.oooooo.....oooooo.XXXXX.                 ",
"                .XXXXX.oooooo.....oooooo.XXXXX.                 ",
"               .XXXXX.ooooooo.....ooooooo.XXXXX.                ",
"               .XXXXX.ooooooo.....ooooooo.XXXXX.                ",
"              .XXXXX.oooooooo.....oooooooo.XXXXX.               ",
"              .XXXXX.ooooooooo...ooooooooo.XXXXX.               ",
"             .XXXXX.oooooooooo...oooooooooo.XXXXX.              ",
"             .XXXXX.oooooooooo...oooooooooo.XXXXX.              ",
"            .XXXXX.ooooooooooo...ooooooooooo.XXXXX.             ",
"            .XXXXX.ooooooooooo...ooooooooooo.XXXXX.             ",
"           .XXXXX.oooooooooooo...oooooooooooo.XXXXX.            ",
"           .XXXXX.ooooooooooooooooooooooooooo.XXXXX.            ",
"          .XXXXX.ooooooooooooooooooooooooooooo.XXXXX.           ",
"          .XXXXX.ooooooooooooo...ooooooooooooo.XXXXX.           ",
"         .XXXXX.oooooooooooooo...oooooooooooooo.XXXXX.          ",
"         .XXXXX.oooooooooooooo...oooooooooooooo.XXXXX.          ",
"        .XXXXX.ooooooooooooooo...ooooooooooooooo.XXXXX.         ",
"        .XXXXX.ooooooooooooooooooooooooooooooooo.XXXXX.         ",
"       .XXXXX.ooooooooooooooooooooooooooooooooooo.XXXXX.        ",
"       .XXXXX.ooooooooooooooooooooooooooooooooooo.XXXXX.        ",
"      .XXXXX.ooooooooooooooooooooooooooooooooooooo.XXXXX.       ",
"      .XXXXX.......................................XXXXX.       ",
"     .XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.      ",
"     .XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.      ",
"    .XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.     ",
"    .XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.     ",
"   .XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.    ",
"   .........................................................    ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                "};

static char *question_xpm [] = {
"64 64 4 1",
" 	s None c None",
".	c #000000000000",
"X	c #51445144FBEE",
"o	c #FFFFFFFFFFFF",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                           ..........                           ",
"                       ....XXXXXXXXXX....                       ",
"                     ..XXXXXXXXXXXXXXXXXX..                     ",
"                   ..XXXXXXXXXXXXXXXXXXXXXX..                   ",
"                 ..XXXXXXXXXXXXXXXXXXXXXXXXXX..                 ",
"                .XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.                ",
"               .XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.               ",
"              .XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.              ",
"             .XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.             ",
"            .XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.            ",
"           .XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.           ",
"          .XXXXXXXXXXXXXXXXXX......XXXXXXXXXXXXXXXXXX.          ",
"         .XXXXXXXXXXXXXXXXX..oooooo..XXXXXXXXXXXXXXXXX.         ",
"         .XXXXXXXXXXXXXXXX.oooooooooo.XXXXXXXXXXXXXXXX.         ",
"        .XXXXXXXXXXXXXXXX.oooooooooooo.XXXXXXXXXXXXXXXX.        ",
"        .XXXXXXXXXXXXXXX.oooooooooooooo.XXXXXXXXXXXXXXX.        ",
"       .XXXXXXXXXXXXXXX.oooooo....oooooo.XXXXXXXXXXXXXXX.       ",
"       .XXXXXXXXXXXXXXX.ooooo.XXXX.ooooo.XXXXXXXXXXXXXXX.       ",
"      .XXXXXXXXXXXXXXX.ooooo.XXXXXX.ooooo.XXXXXXXXXXXXXXX.      ",
"      .XXXXXXXXXXXXXXX.oooo.XXXXXXXX.oooo.XXXXXXXXXXXXXXX.      ",
"      .XXXXXXXXXXXXXXXX.oo.XXXXXXXXX.oooo.XXXXXXXXXXXXXXX.      ",
"      .XXXXXXXXXXXXXXXX....XXXXXXXXX.oooo.XXXXXXXXXXXXXXX.      ",
"     .XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.oooo.XXXXXXXXXXXXXXXX.     ",
"     .XXXXXXXXXXXXXXXXXXXXXXXXXXXXX.ooooo.XXXXXXXXXXXXXXXX.     ",
"     .XXXXXXXXXXXXXXXXXXXXXXXXXXXX.ooooo.XXXXXXXXXXXXXXXXX.     ",
"     .XXXXXXXXXXXXXXXXXXXXXXXXXXX.oooooo.XXXXXXXXXXXXXXXXX.     ",
"     .XXXXXXXXXXXXXXXXXXXXXXXXXX.oooooo.XXXXXXXXXXXXXXXXXX.     ",
"     .XXXXXXXXXXXXXXXXXXXXXXXXX.oooooo.XXXXXXXXXXXXXXXXXXX.     ",
"     .XXXXXXXXXXXXXXXXXXXXXXXX.oooooo.XXXXXXXXXXXXXXXXXXXX.     ",
"     .XXXXXXXXXXXXXXXXXXXXXXX.oooooo.XXXXXXXXXXXXXXXXXXXXX.     ",
"     .XXXXXXXXXXXXXXXXXXXXXXX.ooooo.XXXXXXXXXXXXXXXXXXXXXX.     ",
"     .XXXXXXXXXXXXXXXXXXXXXXX.oooo.XXXXXXXXXXXXXXXXXXXXXXX.     ",
"      .XXXXXXXXXXXXXXXXXXXXXX.oooo.XXXXXXXXXXXXXXXXXXXXXX.      ",
"      .XXXXXXXXXXXXXXXXXXXXXX.oooo.XXXXXXXXXXXXXXXXXXXXXX.      ",
"      .XXXXXXXXXXXXXXXXXXXXXX.oooo.XXXXXXXXXXXXXXXXXXXXXX.      ",
"      .XXXXXXXXXXXXXXXXXXXXXX.oooo.XXXXXXXXXXXXXXXXXXXXXX.      ",
"       .XXXXXXXXXXXXXXXXXXXXXX....XXXXXXXXXXXXXXXXXXXXXX.       ",
"       .XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.       ",
"        .XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.        ",
"        .XXXXXXXXXXXXXXXXXXXXX....XXXXXXXXXXXXXXXXXXXXX.        ",
"         .XXXXXXXXXXXXXXXXXXX.oooo.XXXXXXXXXXXXXXXXXXX.         ",
"         .XXXXXXXXXXXXXXXXXXX.oooo.XXXXXXXXXXXXXXXXXXX.         ",
"          .XXXXXXXXXXXXXXXXXX.oooo.XXXXXXXXXXXXXXXXXX.          ",
"           .XXXXXXXXXXXXXXXXX.oooo.XXXXXXXXXXXXXXXXX.           ",
"            .XXXXXXXXXXXXXXXXX....XXXXXXXXXXXXXXXXX.            ",
"             .XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.             ",
"              .XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.              ",
"               .XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.               ",
"                .XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.                ",
"                 ..XXXXXXXXXXXXXXXXXXXXXXXXXX..                 ",
"                   ..XXXXXXXXXXXXXXXXXXXXXX..                   ",
"                     ..XXXXXXXXXXXXXXXXXX..                     ",
"                       ....XXXXXXXXXX....                       ",
"                           ..........                           ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                "};

static char *kill_xpm [] = {
"43 44 5 1",
" 	c None",
".	c #EFBEEFBEEFBE",
"X	c #514451445144",
"o	c #9E79A2899E79",
"O	c #000000000000",
"   ...XX                           ....oX  ",
"  ...oXO                           ...oXXo ",
" ..oXXOO       XOOOOOOOOO          ..XXoOXo",
"..oXXOOX     XOOXXXOXOXOOOOX       .XXOXoOX",
"..oXXX.X   XOOXXXXXXXXXXOOOOOX    X..oXooXO",
".oXOX...X XOXXXXoXoXoXoXoOXOOOX  Xo..oOXXOO",
"oXOOXoo.XXOXXoXoXoXoooXoXoOXOOOOXo..ooXOOO ",
"XOOXOXooOOXXoXoXoooooooXoXXOXOOOXooooXOXo  ",
"XX  XOXXOXXoX.o...o.o.o.XoOXOXOOOXXoXOX    ",
"     XOOXXoX.o...o.o.o.o.XoOXOXOOOXXOX     ",
"      XOXoXoo.....o.o.o.o.XoOXOXOOXOX      ",
"      OXoXoX.......o.....o.XXOXOOOOX       ",
"      XXXoX.........o.....ooOXOXOOO        ",
"      OXoX.o.......o.....o.XXOXOOOO        ",
"      XXXoo...............ooOXOXOOO        ",
"      OXoX.o.....o.o.o.oX.XXXOXOOOO        ",
"      XOXXXo......o.o.o......oOOOOO        ",
"      OXoo.........o.o....XXXXXOOOX        ",
"      oXooXOOOOOXXo..ooXOOOOOOXoOOX        ",
"      XX.XOOOOOOOX..ooXOOOOOOOOoOO         ",
"      Xo.XOOOOOOOOo..XXOOOOOOOOXOO         ",
"      o..OOOOOOOOO..OXXXOOOOOOOXOO         ",
"      Xo.OOOOOOOOX.OOOXXOOOOOOXXOX         ",
"       X..OOOOOXo..OOOXXXXOOOOoOOX         ",
"       OX.o.o.o...XOOOOXOXXoXoOXO          ",
"        Oo.o.o.o..OOOOOXXOXXoXXOX          ",
"        OXo..o....OOOOOXOXOoXXOOX          ",
"         OXOX.....oOOOXXXOXOOOOO           ",
"         XoXO.o.....ooXoXXXXOXO            ",
"         XX.O.O.o...oXoXXOXOOOOX           ",
"        .Xo.XOOoOoOoOXXOXXOOOXOXX          ",
"       ..XX..XXOXoOoOXOOXOOOXOOOX          ",
"      ..ooXoo.XOoOXOXOXOOXOXOOOOOX         ",
"     ..ooXOX...XoOXOXOXOXOXOXOOOOOX        ",
"    ..ooXOXXXo..o.ooXXXXXXOXOOOOOXOX       ",
" ....ooXOXX XX...o.ooXXOXOXOO XOXoXOX      ",
".....oXOO    Xo.....ooOXOXOOX  XOo.o.XoXOX ",
".....XoO      Xo....oXXOXOOX    XXo.....XOX",
"oo.XX.oXX      OXoooXXOXOOX      XXo..oXoOX",
"XXo...ooO       XOXOXOOOOX        XoooXoXO ",
" oXXo.oXO                          XX.oXOX ",
"  oXXoXOO                          X.oXOX  ",
"   oOOOO                           XoXXX   ",
"     XO                            oXO     "};

/*****************************************************************************

				public code
  
*****************************************************************************/

void
dialog_popup (dialog_e type, const char *text,
	      void (*ok_function) (GtkWidget *, gpointer),
	      gpointer dataptr_ok_function)
{
   GtkWidget *pixmapwid, *hbox;
   GtkWidget *window, *button, *label;
   GdkPixmap *pixmap;
   GdkBitmap *mask;
   GtkStyle  *style;
   char		*window_title[] = {"Info", "Question", "Warning", "Error"};
   char		**window_icon[] = {info_xpm, question_xpm, warning_xpm,
				   kill_xpm};
   
   window = gtk_dialog_new ();
   gtk_window_set_title (GTK_WINDOW (window), window_title [type]);
   gtk_widget_show (window);

   gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		       GTK_SIGNAL_FUNC (gtk_false), NULL);
   gtk_signal_connect_object (GTK_OBJECT (window), "destroy",
			      (GtkSignalFunc) gtk_widget_destroy,
			      GTK_OBJECT (window));

   hbox = gtk_hbox_new (FALSE, 5);
   gtk_widget_show (hbox);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), hbox, FALSE, FALSE,
		       0);
   gtk_container_border_width (GTK_CONTAINER (hbox), 10);
   
   style = gtk_widget_get_style (window);
   pixmap = gdk_pixmap_create_from_xpm_d (window->window, &mask,
					  &style->bg [GTK_STATE_NORMAL],
					  window_icon [type]);
   pixmapwid = gtk_pixmap_new (pixmap, mask);
   gtk_widget_show (pixmapwid);
   gtk_box_pack_start (GTK_BOX (hbox), pixmapwid, TRUE, FALSE, 5);

   label = gtk_label_new (text);
   gtk_widget_show (label);
   gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 5);
   
   button = gtk_button_new_with_label ("OK");
   gtk_widget_show (button);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), button,
		       TRUE, TRUE, 10);
   GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
   gtk_widget_grab_default (button);
   if (ok_function)
   {
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  GTK_SIGNAL_FUNC (ok_function), dataptr_ok_function);
      gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
				 (GtkSignalFunc) gtk_widget_destroy,
				 GTK_OBJECT (window));
      
      button = gtk_button_new_with_label ("Cancel");
      gtk_widget_show (button);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), button,
			  TRUE, TRUE, 10);
   }
   gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			      (GtkSignalFunc) gtk_widget_destroy,
			      GTK_OBJECT (window));

}

gint
delete_window (const GtkWidget *widget, gpointer data)
/*
 *  Delete window.
 *
 *  Return value:
 *	TRUE	i.e. GTK will emit the "destroy" signal
 */
{
   return (TRUE); 
}

gint
hide_window (GtkWidget *widget)
/*
 *  Hide window.
 *
 *  Return value:
 *	FALSE	i.e. GTK will not emit the "destroy" signal
 */
{
   gtk_widget_hide (widget);
   return (FALSE); 
}

void
destroy_window (GtkWidget  *widget, GtkWidget **window)
{
   gtk_widget_hide (*window);
   gtk_widget_destroy (*window);
   *window = NULL;
}

void
destroy_application (const GtkWidget *widget, gpointer data)
/*
 *  Return from gtk_main () loop, i.e. leave the application.
 *
 *  No return value.
 */
{
   gtk_main_quit (); 
}




