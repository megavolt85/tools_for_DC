#include "ui.h"
#include <FL/Fl_File_Chooser.H>

class AFSToolsWindow : public AFSToolsUI
{
public:
	AFSToolsWindow();
	void show();
	int cur_mode;
	
	int get_cur_mode() { return cur_mode; }
	void set_cur_mode(int m) { cur_mode = m; }
};

extern AFSToolsWindow *win;
extern Progress_Bar *pb;
extern char l_pbar1[8], l_pbar2[16];
