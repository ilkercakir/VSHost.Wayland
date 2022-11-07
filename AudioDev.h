#ifndef AudioDevH
#define AudioDevH

#include <stdio.h>

#include <gtk/gtk.h>
#include <alsa/asoundlib.h>

void populate_card_list(GtkWidget *comboinputdev, GtkWidget *combooutputdev);
void populate_input_devices_list(GtkWidget *comboinputdev);
void populate_output_devices_list(GtkWidget *combooutputdev);


#endif
