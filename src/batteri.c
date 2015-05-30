/*
gcc `pkg-config gtk+-x11-2.0 --cflags` `pkg-config upower-glib --cflags` `pkg-config gobject-2.0 --cflags` -finline-small-functions -ffunction-sections -fdata-sections -fmerge-all-constants -fomit-frame-pointer -mno-accumulate-outgoing-args -fno-unwind-tables -fno-asynchronous-unwind-tables -Os batteri.c -o batteri -lgobject-2.0 -lglib-2.0 -lgio-2.0 -lupower-glib -lgtk-x11-2.0 -lgdk-x11-2.0
*/

#include <gtk/gtk.h>
#include <libupower-glib/upower.h>
#include <stdio.h>
#include <stdlib.h>
//sprintf
#include <string.h>
//getopt
#include <unistd.h>

const gchar* icon;
char tooltip_message[150];
int debug = 0;
GtkStatusIcon *si;
const char* path_icon;

//Functions
//gtk tray icon
int tooltip(const gchar* tooltip_message);
int make_icon(const gchar* name_of_icon);
void make_menu(GtkStatusIcon *status_icon, guint button, guint32 activate_time, gpointer menu);
static void about_window();
//battery upower stuff
static void battery_status(UpClient* client);
static void power_device_alteration(UpClient* powerClient, UpDevice* device, gpointer data);
void info_window();
void suspend_computer();
void hibernate_computer();

int main(int argc, char* argv[]){
	int suspend = 0;
	char i=1;
	int optcheck;
	gtk_init (&argc, &argv);
	while((optcheck = getopt (argc, argv, "vsh:")) != -1){//i<argc && argv[i][0]=='-'){
	  switch(optcheck){//argv[i][1]){
		case 'v':
			debug = 1;
			break;
		case 's':
			suspend=1;
			break;
		case 'h':
			g_printerr(
				"usage:\n%s [-d] [-s]\n"
				"  -v	turn Verbose mode on\n"
				"  -s	turn suspend on (if possible)\n"
				,argv[0]);
				return 1;
				break;
		default:
			break;
	  }
	  i++;
	}
	GError* error = NULL;
	UpClient* powerClient = up_client_new();
	gboolean is_lid = up_client_get_lid_is_present (powerClient);
	gboolean can_suspend=up_client_get_can_suspend (powerClient);
	if (!up_client_enumerate_devices_sync(powerClient, NULL, &error)){g_error("unable to enumerate devices - %s", error->message);}
	
	si = gtk_status_icon_new ();
	icon = "battery-good";
	make_icon(icon);
	//GtkImageMenuItem  
	GtkWidget *menu, *menu_hibernate,*menu_suspend, *menu_about, *menu_exit, *menu_info;
	//GtkWidget *menu_suspend_image, *menu_hibernate_image;
	//make the menu
    menu = gtk_menu_new();
    //here are the items
    menu_suspend = gtk_image_menu_item_new_with_label ("Suspend");
    menu_hibernate = gtk_menu_item_new_with_label ("Hibernate");
    menu_about = gtk_image_menu_item_new_from_stock (GTK_STOCK_ABOUT,NULL);
    menu_info = gtk_image_menu_item_new_from_stock (GTK_STOCK_DIALOG_INFO,NULL);
    menu_exit = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT,NULL);
    //menu_suspend_image
    //menu_hibernate_image
    
    //add them to the menu
    gtk_menu_append (GTK_MENU (menu), menu_suspend);
    gtk_menu_append (GTK_MENU (menu), menu_hibernate);
    gtk_menu_append (GTK_MENU (menu), menu_about);
    gtk_menu_append (GTK_MENU (menu), menu_info);
    gtk_menu_append (GTK_MENU (menu), menu_exit); 
    g_signal_connect (G_OBJECT (menu_about), "activate", G_CALLBACK (about_window), NULL);
    g_signal_connect (G_OBJECT (menu_suspend), "activate", G_CALLBACK (suspend_computer), NULL);
    g_signal_connect (G_OBJECT (menu_hibernate), "activate", G_CALLBACK (hibernate_computer), NULL);
    g_signal_connect (G_OBJECT (menu_info), "activate", G_CALLBACK (info_window), NULL);
    g_signal_connect (G_OBJECT (menu_exit), "activate", G_CALLBACK (exit), NULL);
    gtk_widget_show_all (menu);
	g_signal_connect(GTK_STATUS_ICON(si), "popup-menu", GTK_SIGNAL_FUNC(make_menu),menu);
	battery_status(powerClient);
	////notify::lid-is-closed
	if(suspend&&is_lid&&can_suspend){
		g_message("can suspend");
		if(up_client_get_lid_is_closed (powerClient)){g_message("Lid closed");}
		g_signal_connect(powerClient, "notify::lid-is-closed",
					 G_CALLBACK(suspend_computer), NULL);
	}
	g_signal_connect(powerClient, "device-changed",
					 G_CALLBACK(power_device_alteration), NULL);
	g_signal_connect(powerClient, "device-added",
					 G_CALLBACK(power_device_alteration), NULL);
	g_signal_connect(powerClient, "device-removed",
					 G_CALLBACK(power_device_alteration), NULL);
	GMainLoop* loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);

	g_object_unref(powerClient);
	g_object_unref(si);
	g_object_unref(error);

	return 0;
}

void make_menu(GtkStatusIcon *status_icon, guint button, guint32 activate_time, gpointer menu){     
    gtk_menu_popup (GTK_MENU(menu),
                NULL, //parent_menu_shell,
                NULL, //parent_menu_item,
                gtk_status_icon_position_menu, // position function,
                status_icon, //gpointer data,
                button,
                activate_time);	
    
}
void suspend_computer(){
	UpClient* powerClient = up_client_new();
	GError* error = NULL;
	//can it suspend??
	gboolean can_suspend=up_client_get_can_suspend (powerClient);
	if(can_suspend){
		if(up_client_suspend_sync (powerClient,NULL,&error)){
			if(debug){g_message("Suspending");}
		}
	}
	else{
		g_message("Computer cannot suspend.");
	}
}
void hibernate_computer(){
	UpClient* powerClient = up_client_new();
	GError* error = NULL;
	//can it hibernate??
	gboolean can_hibernate=up_client_get_can_hibernate (powerClient);
	if(can_hibernate){
		//tell upower we are going to hibernate
			if(up_client_hibernate_sync (powerClient,NULL,&error)){
				if(debug){g_message("Hibernation");}
			}
		
	}
	else{
		g_message("Computer cannot hibernate.");
	}
}


///BATTERY
static void battery_status(UpClient* client){
	GPtrArray* devices = up_client_get_devices(client);
	if (!devices){
		return;
	}

	// single battery data
	UpDevice* device;
	UpDeviceKind kind;
	UpDeviceState state;
	gboolean is_present;
	gdouble energy, energy_empty, energy_full, energy_rate;

	/*
	 * Combined battery data.
	 */
	int num_battery_devices = 0;
	// Set the initial state to unknown - there might be no battery devices.
	UpDeviceState current_state = 0;
	gdouble current_energy = 0, current_energy_full = 0, current_energy_rate = 0;

	/*
	 * Final battery data.
	 */
	UpDeviceState f_state;
	gdouble f_percentage = 0;
	gdouble f_charging_time = 0;
	gdouble f_discharging_time = 0;
	int i;
	for (i = 0; i < devices->len; i++) {
			device = g_ptr_array_index(devices, i);
			g_object_get(device,
					 "kind", &kind,
					 "state", &state,
					 "is-present", &is_present,
					 "energy", &energy,
					 "energy-empty", &energy_empty,
					 "energy-full", &energy_full,
					 "energy-rate", &energy_rate, NULL);

		// Only take into account present battery devices
		if (kind != UP_DEVICE_KIND_BATTERY || !is_present){continue;}
			num_battery_devices++;

		// Only update the state if the current combined state does not indicate
		// that a battery device is discharging to keep the system running
		//if (current_state != UP_DEVICE_STATE_DISCHARGING)
		//    current_state = state;
		current_state |= (1 << state);

		current_energy += energy - energy_empty;
		current_energy_full += energy_full;
		current_energy_rate += energy_rate;
	}

	g_ptr_array_unref(devices);

	if (current_energy_full > 0)
		f_percentage = current_energy / current_energy_full;

	if (current_state & (1 << UP_DEVICE_STATE_DISCHARGING))
		f_state = UP_DEVICE_STATE_DISCHARGING;
	else if (current_state & (1 << UP_DEVICE_STATE_CHARGING))
		f_state = UP_DEVICE_STATE_CHARGING;
	else if (current_state & (1 << UP_DEVICE_STATE_FULLY_CHARGED))
		f_state = UP_DEVICE_STATE_FULLY_CHARGED;
	else
		f_state = UP_DEVICE_STATE_UNKNOWN;

	float percent = f_percentage * 100.0;
	int percent_int = (int)percent;
///Is the Battery Discharging??
	if (f_state == UP_DEVICE_STATE_DISCHARGING){
		//why doesn't this display useful things?
		//status = up_device_state_to_string(current_state);

		//find time to discharge
		//multiply seconds in an hour by the current energy
		f_discharging_time = (3600.0 * current_energy) / current_energy_rate;
		float f_hours_discharge= (f_discharging_time/60.0)/60; //hours
		
		//set icon based on percent
		if(percent_int >= 80){icon = "battery-full";}
		else if((percent_int >= 50 )&& (percent_int < 80)){icon = "battery-good";}
		else if((percent_int >= 20)&& (percent_int < 50)){icon = "battery-low";}
		else if((percent_int >= 5)&& (percent_int < 20)){icon = "battery-caution";}
		else {icon = "battery-empty";}
		sprintf(tooltip_message, "Percentage: \t%d %%\nDischarging Time:\t %.2f hours",  percent_int, f_hours_discharge);
	}
	else if (f_state == UP_DEVICE_STATE_CHARGING){
		//status = up_device_state_to_string(current_state);
		//
		f_charging_time = 3600.0 * (current_energy_full - current_energy) / current_energy_rate;
		float f_hours_charge= (f_charging_time/60.0)/60; //hours
		//set icon based on percent
		if(percent_int >= 99){icon = "battery-full-charged";}
		else if(percent_int >= 80){icon = "battery-full-charging";} //battery-full-charging";}//-symbolic
		else if((percent_int >= 50) && (percent_int < 80)){icon = "battery-good-charging";}
		else if((percent_int >= 20) && (percent_int < 50)){icon = "battery-low-charging";}
		else if(percent_int >= 5){icon = "battery-caution-charging";}
		else{icon = "battery-empty-charging";}
		sprintf(tooltip_message, "Percentage: \t%d %%\nCharging Time:\t %.2f hours", percent_int, f_hours_charge);
	}
	int result = 0;
	int tool_result = 0;
	if (!num_battery_devices) {
		icon = "battery-empty-charging";
		result = make_icon(icon);
		sprintf(tooltip_message, "No battery present");
		tool_result = tooltip(tooltip_message);
		g_message("\tno battery status data to report, ending");
		return;
	}
	result = make_icon(icon);
	tool_result = tooltip(tooltip_message);
	if (result||tool_result){g_message("issue");}
	if(debug){
		const gchar * kind_of_device = up_device_to_text(device);
		g_message("\t%d battery device(s) present", num_battery_devices);
		g_message(" ");
		g_message("State: %s", up_device_state_to_string(current_state));
		g_message("Percentage(float): %f", f_percentage * 100.0);
		g_message("Percentage(int): %d", percent_int);
		g_message(" ");
		g_message("Charging Time: \t%f", f_charging_time);
		g_message("Discharging Time: \t%f", f_discharging_time);
		g_message("Level: \t%f", f_percentage);
		g_message(" ");
		g_message("Current Icon: %s", icon);
		g_message(" ");
		g_message ("Full Device Report:\n%s", kind_of_device);
		g_message("--- end of report ---");
	}
}

static void power_device_alteration(UpClient* client, UpDevice* device, gpointer data){
	UpDeviceKind kind;
	g_object_get(device,
				 "kind", &kind, NULL);
	if (kind != UP_DEVICE_KIND_BATTERY)
		return;

	battery_status(client);
}

///Battery stuff
int make_icon(const gchar* name_of_icon){
  if (name_of_icon==NULL){return 2;}
	char temp_icon[70];
	sprintf(temp_icon,"%s-symbolic", name_of_icon);
	name_of_icon=temp_icon;
	GdkPixbuf      *pix = NULL;
	pix = gtk_icon_theme_load_icon (gtk_icon_theme_get_default(),
									 name_of_icon,
									32,
									GTK_ICON_LOOKUP_USE_BUILTIN,
									NULL);
	gtk_status_icon_set_from_pixbuf(si,pix);
	return 0;
}

int tooltip(const gchar* tooltip_message){
	gtk_status_icon_set_tooltip_markup(si, tooltip_message);
	return 0;
}
static void about_window(){
	GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "About Batteri");
	gtk_widget_set_size_request (window, 200, 200);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	//gtk_window_set_icon(GTK_WINDOW(window), create_pixbuf("batteri.svg"));
	GtkWidget *label = gtk_label_new("\tBatteri\nA libre open\nGTK/UPower tray icon\nCopyright IsraelDahl 2015 GPL3");
	gtk_container_add (GTK_CONTAINER (window), label);
	gtk_widget_show_all (window);
}
void info_window(){
	UpClient* powerClient = up_client_new();
	UpDevice* device = NULL;
	gboolean is_present;
	const gchar * kind_of_device = NULL;
	GPtrArray* devices = up_client_get_devices(powerClient);
	if (!devices){kind_of_device = "No device Available";}
	else{
		int i;
		for (i = 0; i < devices->len; i++) {
			device = g_ptr_array_index(devices, i);
			UpDeviceKind kind;
			g_object_get(device,"is-present", &is_present,"kind", &kind, NULL);
			// Only take into account present battery devices
			if (kind != UP_DEVICE_KIND_BATTERY || !is_present){continue;}
		}
		kind_of_device = up_device_to_text(device);
	}
	GtkWidget *window = gtk_dialog_new();
	GtkWidget *scroller_window;
	gtk_window_set_title (GTK_WINDOW (window), "Info");//window_add_with_viewport
	gtk_widget_set_size_request (window, 400, 400);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	scroller_window = gtk_scrolled_window_new (NULL,NULL);
	gtk_container_set_border_width (GTK_CONTAINER (scroller_window), 7);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller_window),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG(window)->vbox), scroller_window, 
			TRUE, TRUE, 0);
	//gtk_window_set_icon(GTK_WINDOW(window), create_pixbuf("batteri.svg"));
	GtkWidget *label = gtk_label_new(kind_of_device);
	  gtk_scrolled_window_add_with_viewport (
                   GTK_SCROLLED_WINDOW (scroller_window), label);
	//gtk_container_add (GTK_CONTAINER (window), label);
	gtk_widget_show_all (window);	
}
