FaztWeb - A minimalistic browser based on TazWeb
================================================================================

This project started some time ago, when I started to love TazWeb, but
hated the separation between the search and url bar. It was also an exercise
to learn some gtk programming, and eventually turned to something pretty
useful.
FaztWeb aims to provide several improvements over TazWeb, like an
"omnibar" and more customizability through cli args, while keeping
the very best ones of it older brother: minimal interface, great
responsiveness and speed, security and an "always private-mode" approach.

Some differences:

- FaztWeb removes all the references to SliTaz, which is unuseful out of
that distro.

- FaztWeb features StartPage both as default search engine and as home.
- FaztWeb allows you to change the default home, search engine and/or user agent,
for more info execute:

$ faztweb --help

- FaztWeb doesn't support bookmarks.


Build and install
-----------------
FaztWeb, as TazWeb depends on GTK and libwebkit. To build and test, simply:

$ make

$ ./faztweb

To install:

\# make install

Coding notes
------------

    * To get the filename of a WebKitDownload:
      file = webkit_download_get_suggested_filename (download);
      
    * Check mime-type before download (needs a callback function)
      g_signal_connect (web_view, "mime-type-policy-decision-requested",
              G_CALLBACK (mime_type_decision_cb), webview);

    * Get user config directory path in $HOME:
      config = g_strdup_printf ("%s/.config/tazweb", g_get_home_dir ());

    * Change font of a widget:
      PangoFontDescription *font;
      font = pango_font_description_from_string("8");
      gtk_widget_modify_font(urientry, font);
      pango_font_description_free(font);

    * Change size of a widget:
      gtk_widget_set_size_request(toolbar, 0, 28);

    * Append a string
      g_string_append(string, " - TazWeb");

    * Editable webview
      webkit_web_view_set_editable (webview, TRUE);

================================================================================
