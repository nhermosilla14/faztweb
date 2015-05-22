/*
 * FaztWeb is a simple, clean, minimallistic web browser based on the
 * sources of TazWeb. It aims to be as fast and clean as the original
 * one, but much easier to use, and with even less gui elements.
 * It removes a lot of references to SliTaz, and replace the home with
 * the great StartPage search engine.
 * 
 * Copyright (C) 2011-2014 SliTaz GNU/Linux
 * Copyright (C) 2014 Nicolás Hermosilla P.
 * 
 * Released under BSD License
 * See AUTHORS and LICENSE for detailed information
 *
 */

#include <glib.h>
#include <glib/gi18n.h>
#define GETTEXT_PACKAGE "faztweb"
#include <gtk/gtk.h>
#include <webkit/webkit.h>

#define HOME       g_get_home_dir()
#define CONFIG     g_strdup_printf("%s/.config/faztweb", HOME)
#define BMTXT      g_strdup_printf("%s/bookmarks.txt", CONFIG)

/* Needs AppleWebKit/531.2+ to handle all sites ? */
static gchar *useragent = "Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:27.0) Gecko/20100101 Firefox/27.0";//Pretend to be a rockstar,
static GtkWidget* create_window(WebKitWebView** newwebview);											// and you'll be treated as one.
static GtkWidget *mainwindow, *vbox, *browser, *toolbar;
static WebKitWebView *webview;
static WebKitWebFrame *frame;
static gint count = 0;
static gboolean notoolbar;
static gboolean noquit;
static gboolean kiosk;
static gchar* uri;
static gchar *webhome = "http://www.startpage.com";
static gchar *websearch = "http://startpage.com/do/search?query=%s";

/* Create an icon */
static GdkPixbuf *create_pixbuf(const gchar* image)
{
    GdkPixbuf *pixbuf;
    pixbuf = gdk_pixbuf_new_from_file(image, NULL);
    return pixbuf;
}

/* Can be: http://hg.slitaz.org or hg.slitaz.org or anything else :D */
static void check_requested_uri()
{
    if (g_strrstr(uri, "://")) {
        uri = g_strdup(uri);
    } else {
        if (g_strrstr(uri, " ") || (!(g_strrstr(uri, ".")))) {
            int i;
            uri = g_strstrip(uri);
            for (i = 0; (i < strlen(uri)-1); i++) {
                if (uri[i] == " ") {
                    uri[i] = "+";
                }
            }
            uri = g_strdup_printf(websearch, uri);
        } else {
            uri = g_strdup_printf("http://%s", uri);
        }
    }
    //uri = g_strrstr(uri, "://") ? g_strdup(uri)
    //	: g_strdup_printf("http://%s", uri);
}

/* Update title */
static void update_title(GtkWindow* window, WebKitWebView* webview)
{
    GString *string;
    gdouble progress;
    gchar *title;

    string = g_string_new(webkit_web_view_get_title(webview));
    progress = webkit_web_view_get_progress(webview) * 100;
    if (progress < 100)
        g_string_append_printf(string, " [ %.f%% ] ", progress);

    title = g_string_free(string, FALSE);
    gtk_window_set_title(window, title);
    g_free(title);
}

/* Get the page title */
static void notify_title_cb(WebKitWebView* webview, GParamSpec* pspec, GtkWidget* window)
{
    update_title(GTK_WINDOW(window), webview);
}

/* Request progress in window title */
static void notify_progress_cb(WebKitWebView* webview, GParamSpec* pspec, GtkWidget* window)
{
    update_title(GTK_WINDOW(window), webview);
}

/* Notify url entry */
static void notify_load_status_cb(WebKitWebView* webview, GParamSpec* pspec, GtkWidget* urientry)
{
    if (webkit_web_view_get_load_status(webview) == WEBKIT_LOAD_COMMITTED) {
        frame = webkit_web_view_get_main_frame(webview);
        uri = webkit_web_frame_get_uri(frame);
        if (uri)
            gtk_entry_set_text(GTK_ENTRY(urientry), uri);
    }
}

/* Destroy the window */
static void destroy_cb(GtkWidget* widget, GtkWindow* window)
{
    if (g_atomic_int_dec_and_test(&count))
        gtk_main_quit();
}

/* Show page source */
static void view_source_cb(GtkWidget* widget, WebKitWebView* webview)
{
    gboolean source;

    frame = webkit_web_view_get_main_frame(webview);
    uri = webkit_web_frame_get_uri(frame);
    source = webkit_web_view_get_view_source_mode(webview);

    webkit_web_view_set_view_source_mode(webview, !source);
    webkit_web_view_reload(webview);
}

/* URL and search entry callback function */
static void uri_entry_cb(GtkWidget* urientry, WebKitWebView* webview)
{
    uri = gtk_entry_get_text(GTK_ENTRY(urientry));
    g_assert(uri);
    check_requested_uri();
    webkit_web_view_load_uri(webview, uri);
}

/* Search entry and icon callback function */
/*static void search_web(GtkWidget* search, WebKitWebView* webview)
{
	uri = g_strdup_printf(websearch, gtk_entry_get_text(GTK_ENTRY(search)));
	g_assert(uri);
	webkit_web_view_load_uri(webview, uri);
}

static void search_entry_cb(GtkWidget* search, WebKitWebView* webview)
{
	search_web(search, webview);
}

static void search_icon_press_cb(GtkWidget *search, GtkEntryIconPosition pos,
		GdkEvent *event, WebKitWebView* webview)
{
    search_web(search, webview);
}
*/
/* Navigation button function: */
//static void
//go_bookmarks_cb(GtkWidget* widget, WebKitWebView* webview)
//{
//	uri = g_strdup_printf("%s?home=%s", BMURL, HOME);
//	g_assert(uri);
//	webkit_web_view_load_uri(webview, uri);
//}

static void go_home_cb(GtkWidget* widget, WebKitWebView* webview)
{
    uri = webhome;
    g_assert(uri);
    webkit_web_view_load_uri(webview, uri);
}

static void go_back_cb(GtkWidget* widget, WebKitWebView* webview)
{
    webkit_web_view_go_back(webview);
}

static void go_forward_cb(GtkWidget* widget, WebKitWebView* webview)
{
    webkit_web_view_go_forward(webview);
}

/* Fullscreen and unfullscreen callback function */
static void fullscreen_cb(GtkWindow* window, gpointer data)
{
    GdkWindowState state;
    state = gdk_window_get_state(gtk_widget_get_window(GTK_WIDGET(mainwindow)));

    if(state & GDK_WINDOW_STATE_FULLSCREEN)
        gtk_window_unfullscreen(GTK_WINDOW(mainwindow));
    else
        gtk_window_fullscreen(GTK_WINDOW(mainwindow));
}

/* FaztWeb doc callback function */
//static void
//faztweb_doc_cb(GtkWidget* widget, WebKitWebView *webview)
//{
//	uri = ("file:///usr/share/doc/faztweb/faztweb.html");
//	g_assert(uri);
//	webkit_web_view_load_uri(webview, uri);
//}

/* Download function */
static gboolean download_requested_cb(WebKitWebView *webview, WebKitDownload *download, gpointer user_data)
{
    gchar* buffer;
    uri = webkit_download_get_uri(download);
    asprintf(&buffer, "xterm -T 'Descargas' -geom 72x10+0-24 -e \
				'mkdir -p $HOME/Escritorio && cd $HOME/Escritorio && \
				wget -c %s; sleep 2' &", uri);
    system(buffer);
}

/* Zoom out and in callback function */
static void zoom_out_cb(GtkWidget *widget, WebKitWebView* webview)
{
    webkit_web_view_zoom_out(webview);
}

static void zoom_in_cb(GtkWidget *widget, WebKitWebView* webview)
{
    webkit_web_view_zoom_in(webview);
}

/* Printing callback function */
static void print_page_cb (GtkWidget *widget, gpointer data)
{
    webkit_web_frame_print (webkit_web_view_get_main_frame (
                                WEBKIT_WEB_VIEW (webview)));
}

/* New webview callbacks */
static WebKitWebView* create_web_view_cb(WebKitWebView* webview, WebKitWebFrame* web_frame, GtkWidget* window)
{
    WebKitWebView *newview;
    create_window(&newview);
    return newview;
}

static gboolean webview_ready_cb(WebKitWebView* webview, GtkWidget* window)
{
    gtk_widget_grab_focus(GTK_WIDGET(webview));
    gtk_widget_show_all(window);
    return FALSE;
}

static gboolean close_webview_cb(WebKitWebView* webview, GtkWidget* window)
{
    gtk_widget_destroy(window);
    return TRUE;
}

/* Add items to WebKit contextual menu */
static void populate_menu_cb(WebKitWebView *webview, GtkMenu *menu, gpointer data)
{
    GtkWidget* item;

    /* Separator */
    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    /* Zoom in */
    item = gtk_image_menu_item_new_with_label(_("Zoom in"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
                                  gtk_image_new_from_stock(GTK_STOCK_ZOOM_IN, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(item, "activate", G_CALLBACK(zoom_in_cb), webview);

    /* Zoom out */
    item = gtk_image_menu_item_new_with_label(_("Zoom out"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
                                  gtk_image_new_from_stock(GTK_STOCK_ZOOM_OUT, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(item, "activate", G_CALLBACK(zoom_out_cb), webview);

    /* Separator */
    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);


    /* Printing */
    item = gtk_image_menu_item_new_with_label(_("Print this page"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
                                  gtk_image_new_from_stock(GTK_STOCK_PRINT, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(item, "activate", G_CALLBACK(print_page_cb), webview);

    /* View source mode */
    item = gtk_image_menu_item_new_with_label(_("View source mode"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
                                  gtk_image_new_from_stock(GTK_STOCK_PROPERTIES, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(item, "activate", G_CALLBACK(view_source_cb), webview);
    
    /* Toggle fullscreen mode */
	if (! kiosk) {
		item = gtk_image_menu_item_new_with_label(_("Toggle fullscreen mode"));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
									gtk_image_new_from_stock(GTK_STOCK_FULLSCREEN, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(item, "activate", G_CALLBACK(fullscreen_cb), NULL);
	}
	
    /* Separator */
    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    /* Quit FaztWeb */
    item = gtk_image_menu_item_new_with_label(_("Quit FaztWeb"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
                                  gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(item, "activate", G_CALLBACK(destroy_cb), webview);

    gtk_widget_show_all(GTK_WIDGET(menu));
}

/* Scrolled window for the webview */
static GtkWidget* create_browser(GtkWidget* window, GtkWidget* urientry, WebKitWebView* webview)
{
    WebKitWebSettings *settings;

    browser = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(browser),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    gtk_container_add(GTK_CONTAINER(browser), GTK_WIDGET(webview));

    /* User agent */
    settings = webkit_web_view_get_settings (webview);
    g_object_set(G_OBJECT(settings), "user-agent", useragent, NULL);

    /* Connect WebKit events */
    g_signal_connect(webview, "notify::title",
                     G_CALLBACK(notify_title_cb), window);
    g_signal_connect(webview, "notify::progress",
                     G_CALLBACK(notify_progress_cb), window);
    g_signal_connect(webview, "notify::load-status",
                     G_CALLBACK(notify_load_status_cb), urientry);
    g_signal_connect(webview, "web-view-ready",
                     G_CALLBACK(webview_ready_cb), window);
    g_signal_connect(webview, "close-web-view",
                     G_CALLBACK(close_webview_cb), window);

    /* Impossible to open in new window or download in kiosk mode */
    if (! kiosk) {
        g_signal_connect(webview, "download-requested",
                         G_CALLBACK(download_requested_cb), NULL);
        g_signal_connect(webview, "create-web-view",
                         G_CALLBACK(create_web_view_cb), window);
    }

    /* Connect WebKit contextual menu items */
    g_object_connect(G_OBJECT(webview), "signal::populate-popup",
                     G_CALLBACK(populate_menu_cb), webview, NULL);

    return browser;
}

static GtkWidget* create_toolbar(GtkWidget* urientry, WebKitWebView* webview)
{
    GtkToolItem* item;

    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar),
                                GTK_ORIENTATION_HORIZONTAL);
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar),
                          GTK_TOOLBAR_BOTH_HORIZ);

    /* The back button */
    item = gtk_tool_button_new_from_stock(GTK_STOCK_GO_BACK);
    g_signal_connect(G_OBJECT(item), "clicked",
                     G_CALLBACK(go_back_cb), webview);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

    /* The forward button */
    item = gtk_tool_button_new_from_stock(GTK_STOCK_GO_FORWARD);
    g_signal_connect(G_OBJECT(item), "clicked",
                     G_CALLBACK(go_forward_cb), webview);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

    /* Home button */
    item = gtk_tool_button_new_from_stock(GTK_STOCK_HOME);
    g_signal_connect(G_OBJECT(item), "clicked",
                     G_CALLBACK(go_home_cb), webview);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

    /* URL entry */
    item = gtk_tool_item_new();
    gtk_tool_item_set_expand(item, TRUE);
    gtk_widget_set_size_request(urientry, 0, 20);
    gtk_container_add(GTK_CONTAINER(item), urientry);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
    g_signal_connect(G_OBJECT(urientry), "activate",
                     G_CALLBACK(uri_entry_cb), webview);

    return toolbar;
}

/* Main window */
static GtkWidget *create_window(WebKitWebView** newwebview)
{
    GtkWidget *window;
    GtkWidget *urientry;
    //GtkWidget *search;

    g_atomic_int_inc(&count);

    /* Default FaztWeb window */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    gtk_window_set_icon(GTK_WINDOW(window),
                        create_pixbuf("/usr/share/pixmaps/faztweb.png"));
    gtk_widget_set_name(window, "FaztWeb");
    g_signal_connect(window, "destroy", G_CALLBACK(destroy_cb), NULL);

    /* Webview and widgets */
    webview = WEBKIT_WEB_VIEW(webkit_web_view_new());
    urientry = gtk_entry_new();
    //search = gtk_entry_new();
    vbox = gtk_vbox_new(FALSE, 0);

    /* Pack box and container */
    if (! notoolbar)
        gtk_box_pack_start(GTK_BOX(vbox),
                           create_toolbar(urientry, webview), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox),
                       create_browser(window, urientry, webview), TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(window), vbox);

    if (newwebview)
        *newwebview = webview;

    return window;
}

int main(int argc, char *argv[])
{
    textdomain (GETTEXT_PACKAGE);
	
    while (argc > 1) {
        if (!strcmp(argv[1],"--notoolbar")) {
            notoolbar++;
        } else if (!strcmp(argv[1],"--kiosk")) {
            kiosk++;
        } else if (!strcmp(argv[1],"--home") && argc > 2) {
            argc--;
            argv++;
            webhome = g_strrstr(argv[1], "://") ? g_strdup(argv[1]) : g_strdup_printf("http://%s", argv[1]);
        } else if (!strcmp(argv[1],"--search") && argc > 2) {
            argc--;
            argv++;
            websearch = g_strrstr(argv[1], "://") ? g_strdup(argv[1]) : g_strdup_printf("http://%s", argv[1]);
        } else if (!strcmp(argv[1],"--useragent") && argc > 2) {
            argc--;
            argv++;
            useragent = argv[1];
        } else if (!strcmp(argv[1],"--help")) {
            printf ("Usage: faztweb [--notoolbar|--kiosk|--home [home] |--useragent [ua] | --search [url with %%s at search terms] ]\n");
            //printf ("Bookmarks: %s\n", BMTXT);
            return 0;
        } else break;
        argc--;
        argv++;
    }
    gtk_init(NULL, NULL);
    
    /* Load the start page file or the url in argument */
    uri =(char*)(argc > 1 ? argv[1] : webhome);
    if (argv[1])
        check_requested_uri();
    mainwindow = create_window(&webview);
    gtk_widget_show_all(mainwindow);

    /* Fullscreen for Kiosk mode */
    if (kiosk)
        gtk_window_fullscreen(GTK_WINDOW(mainwindow));

    webkit_web_view_load_uri(webview, uri);
    gtk_widget_grab_focus(GTK_WIDGET(webview));
    gtk_main();

    return 0;
}
