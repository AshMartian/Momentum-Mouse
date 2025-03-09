// gui/inertia_scroller_gui.c
// Requires GTK development libraries: apt-get install libgtk-3-dev
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_GROUP "smooth_scroll"

// Default values
#define DEFAULT_SENSITIVITY 1.0
#define DEFAULT_MULTIPLIER  1.0
#define DEFAULT_FRICTION    2.0
#define DEFAULT_MAX_VELOCITY 0.8

// System-wide config file path
#define SYSTEM_CONFIG_FILE "/etc/inertia_scroller.conf"

// Function declarations
static GKeyFile* load_config(void);
static void save_config(GKeyFile *key_file);
static gchar* get_config_file_path(void);

// Function to handle apply button click
static void on_apply_clicked(GtkWidget *widget, gpointer data) {
    (void)widget; // Suppress unused parameter warning
    
    // Retrieve widgets from the grid.
    GtkWidget **widgets = (GtkWidget **)data;
    GtkWidget *sens_scale = widgets[0];
    GtkWidget *mult_scale = widgets[1];
    GtkWidget *fric_scale = widgets[2];
    GtkWidget *vel_scale = widgets[3];

    gdouble sensitivity = gtk_range_get_value(GTK_RANGE(sens_scale));
    gdouble multiplier = gtk_range_get_value(GTK_RANGE(mult_scale));
    gdouble friction = gtk_range_get_value(GTK_RANGE(fric_scale));
    gdouble max_velocity = gtk_range_get_value(GTK_RANGE(vel_scale));

    // Load existing config, update values, and save.
    GKeyFile *config = load_config();
    g_key_file_set_double(config, CONFIG_GROUP, "sensitivity", sensitivity);
    g_key_file_set_double(config, CONFIG_GROUP, "multiplier", multiplier);
    g_key_file_set_double(config, CONFIG_GROUP, "friction", friction);
    g_key_file_set_double(config, CONFIG_GROUP, "max_velocity", max_velocity);
    save_config(config);
    g_key_file_free(config);
}

// Load settings from config file into a GKeyFile structure.
static GKeyFile* load_config(void) {
    GKeyFile *key_file = g_key_file_new();
    GError *error = NULL;
    if (!g_key_file_load_from_file(key_file, SYSTEM_CONFIG_FILE, G_KEY_FILE_NONE, &error)) {
        // File might not exist yet; ignore error and continue with defaults.
        g_clear_error(&error);
    }
    return key_file;
}

// Get the path to the configuration file
static gchar* get_config_file_path(void) {
    // Simply return the system config file path
    return g_strdup(SYSTEM_CONFIG_FILE);
}

// Save settings from key_file back to disk.
static void save_config(GKeyFile *key_file) {
    GError *error = NULL;
    gchar *data = g_key_file_to_data(key_file, NULL, &error);
    if (error) {
        g_printerr("Error converting config to  %s\n", error->message);
        g_error_free(error);
        return;
    }
    
    // Create a temporary file
    gchar *temp_file = g_build_filename(g_get_tmp_dir(), "inertia_scroller.conf.tmp", NULL);
    if (!g_file_set_contents(temp_file, data, -1, &error)) {
        g_printerr("Error creating temporary config file: %s\n", error->message);
        g_error_free(error);
        g_free(temp_file);
        g_free(data);
        return;
    }
    
    // Use pkexec to copy the file to /etc with root permissions AND restart the service in one command
    gchar *command = g_strdup_printf("pkexec sh -c 'cp %s %s && systemctl restart inertia_scroller.service'", 
                                     temp_file, SYSTEM_CONFIG_FILE);
    gint exit_status;
    if (!g_spawn_command_line_sync(command, NULL, NULL, &exit_status, &error)) {
        g_printerr("Error executing pkexec: %s\n", error->message);
        g_error_free(error);
    } else if (exit_status != 0) {
        g_printerr("pkexec command failed with exit status %d\n", exit_status);
    }
    
    // Clean up
    g_free(command);
    g_unlink(temp_file);  // Delete the temporary file
    g_free(temp_file);
    g_free(data);
    
    g_print("Settings saved and service restarted.\n");
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Load configuration
    GKeyFile *config = load_config();

    // Read existing values or fall back to defaults.
    gdouble sensitivity = g_key_file_get_double(config, CONFIG_GROUP, "sensitivity", NULL);
    if (sensitivity == 0) sensitivity = DEFAULT_SENSITIVITY;
    gdouble multiplier = g_key_file_get_double(config, CONFIG_GROUP, "multiplier", NULL);
    if (multiplier == 0) multiplier = DEFAULT_MULTIPLIER;
    gdouble friction = g_key_file_get_double(config, CONFIG_GROUP, "friction", NULL);
    if (friction == 0) friction = DEFAULT_FRICTION;
    gdouble max_velocity = g_key_file_get_double(config, CONFIG_GROUP, "max_velocity", NULL);
    if (max_velocity == 0) max_velocity = DEFAULT_MAX_VELOCITY;

    // Create main window.
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Inertia Scroller Settings");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Create a grid for layout.
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    gtk_container_add(GTK_CONTAINER(window), grid);

    // Sensitivity slider
    GtkWidget *sens_label = gtk_label_new("Sensitivity:");
    gtk_widget_set_halign(sens_label, GTK_ALIGN_END);
    GtkWidget *sens_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.1, 3.0, 0.1);
    gtk_range_set_value(GTK_RANGE(sens_scale), sensitivity);
    gtk_widget_set_hexpand(sens_scale, TRUE);
    gtk_widget_set_halign(sens_scale, GTK_ALIGN_FILL);
    gtk_widget_set_tooltip_text(sens_scale, 
        "The strength of each mouse scroll wheel turn. Higher values make each scroll input have more effect on velocity.");
    gtk_grid_attach(GTK_GRID(grid), sens_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), sens_scale, 1, 0, 1, 1);

    // Multiplier slider
    GtkWidget *mult_label = gtk_label_new("Multiplier:");
    gtk_widget_set_halign(mult_label, GTK_ALIGN_END);
    GtkWidget *mult_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.1, 3.0, 0.1);
    gtk_range_set_value(GTK_RANGE(mult_scale), multiplier);
    gtk_widget_set_hexpand(mult_scale, TRUE);
    gtk_widget_set_halign(mult_scale, GTK_ALIGN_FILL);
    gtk_widget_set_tooltip_text(mult_scale, 
        "The consecutive scroll multiplier. Higher values make repeated scrolling accelerate faster.");
    gtk_grid_attach(GTK_GRID(grid), mult_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), mult_scale, 1, 1, 1, 1);

    // Friction slider
    GtkWidget *fric_label = gtk_label_new("Friction:");
    gtk_widget_set_halign(fric_label, GTK_ALIGN_END);
    GtkWidget *fric_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.1, 5.0, 0.1);
    gtk_range_set_value(GTK_RANGE(fric_scale), friction);
    gtk_widget_set_hexpand(fric_scale, TRUE);
    gtk_widget_set_halign(fric_scale, GTK_ALIGN_FILL);
    gtk_widget_set_tooltip_text(fric_scale, 
        "The rate at which scrolling slows down over time. Higher values make scrolling stop quicker, lower values make it glide longer.");
    gtk_grid_attach(GTK_GRID(grid), fric_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), fric_scale, 1, 2, 1, 1);

    // Max Velocity slider
    GtkWidget *vel_label = gtk_label_new("Max Velocity:");
    gtk_widget_set_halign(vel_label, GTK_ALIGN_END);
    GtkWidget *vel_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.2, 2.0, 0.1);
    gtk_range_set_value(GTK_RANGE(vel_scale), max_velocity);
    gtk_widget_set_hexpand(vel_scale, TRUE);
    gtk_widget_set_halign(vel_scale, GTK_ALIGN_FILL);
    gtk_widget_set_tooltip_text(vel_scale, 
        "The maximum speed that inertia scrolling can reach. Limits how fast content can scroll.");
    gtk_grid_attach(GTK_GRID(grid), vel_label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), vel_scale, 1, 3, 1, 1);

    // Apply button
    GtkWidget *apply_button = gtk_button_new_with_label("Apply");
    gtk_widget_set_hexpand(apply_button, TRUE);
    gtk_widget_set_halign(apply_button, GTK_ALIGN_FILL);
    gtk_grid_attach(GTK_GRID(grid), apply_button, 0, 4, 2, 1);
    
    // Create an array of widget pointers to pass as data
    GtkWidget *widgets[] = {
        sens_scale, mult_scale, fric_scale, vel_scale
    };
    g_signal_connect(apply_button, "clicked", G_CALLBACK(on_apply_clicked), widgets);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
