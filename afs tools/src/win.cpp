#include "win.h"
#include <unistd.h> 

#define VERSION "0.2"

extern char *extnamelist;
extern char nofn;
OF_Choiser *fc;
Progress_Bar *pb;
char l_pbar1[8], l_pbar2[16];
const char *file = NULL;

extern int unpack_afs(const char *fn);
extern int pack_afs(const char *fn);
extern int unpack_folder(const char *fn);
extern int pack_folder(const char *fn);

void cb_btn_pack_callback(Fl_Widget *btn, void* userdata)
{
	if (((AFSToolsWindow*)userdata)->get_cur_mode())
		return;
	
	((AFSToolsWindow*)userdata)->set_cur_mode(1);
	
	((AFSToolsWindow*)userdata)->fbrowser1->filetype(Fl_File_Browser::FILES);
	((AFSToolsWindow*)userdata)->fbrowser1->filter("*.lst");
	((AFSToolsWindow*)userdata)->fbrowser1->load("workdir/");
	
	win->run_btn->label("Упаковать");
	
	if (!win->btn_all_files->value())
	{
		win->run_btn->deactivate();
	}
#ifdef DEBUG
	printf("упаковать\n");
#endif
}

void cb_btn_unpack_callback(Fl_Widget *btn, void* userdata)
{
	if (!((AFSToolsWindow*)userdata)->get_cur_mode())
		return;
	
	((AFSToolsWindow*)userdata)->set_cur_mode(0);
	
	((AFSToolsWindow*)userdata)->fbrowser1->filetype(Fl_File_Browser::FILES);
	((AFSToolsWindow*)userdata)->fbrowser1->filter("*.AFS");
	((AFSToolsWindow*)userdata)->fbrowser1->load("infiles/");
	
	win->run_btn->label("Распаковать");
	
	if (!win->btn_all_files->value())
	{
		win->run_btn->deactivate();
	}
#ifdef DEBUG
	printf("распаковать\n");
#endif
}

void cb_fsel1(Fl_File_Browser *fb, void* userdata)
{
	int i, ln = fb->size();
	
	for (i = 0; i <= ln; i++)
	{
		if (fb->selected(i))
			break;
	}
	
	if (i > ln)
	{
		win->run_btn->deactivate();
		file = NULL;
		return;
	}
	
	win->run_btn->activate();
	
	file = fb->text(i);
#ifdef DEBUG
	printf("%s\n", fb->text(i));
#endif
}

void cb_conv_all(Fl_Button *btn)
{
	switch (btn->value())
	{
		case 1:
			win->fbrowser1->deactivate();
			win->run_btn->activate();
			win->rb_fl->deactivate();
			
			if (extnamelist)
			{
				free(extnamelist);
				extnamelist = NULL;
				win->rb_fl->value(0);
				win->rb_ff->value(1);
			}
			break;
		
		default:
			for (int i = win->fbrowser1->size(); i ; i--)
			{
				if (win->fbrowser1->selected(i))
					win->fbrowser1->select(i, 0);
			}
			
			win->fbrowser1->activate();
			win->run_btn->deactivate();
			win->rb_fl->activate();
			break;
	}
#ifdef DEBUG
	printf("%s\n", ((const char *[]){ "Выкл", "Вкл" })[btn->value()]);
#endif
}

void cb_name_sourse(Fl_Round_Button *btn)
{
	if (btn == win->rb_ff)
	{
		nofn = 0;
		
		if (extnamelist)
		{
			free(extnamelist);
			extnamelist = NULL;
		}
	}
	else if (btn == win->rb_nn)
	{
		nofn = 1;
		
		if (extnamelist)
		{
			free(extnamelist);
			extnamelist = NULL;
		}
	}
	else
	{
		fc->show();

		while (fc->visible())
			Fl::wait();
		
		if (!fc->value())
		{
			nofn = 0;
		
			if (extnamelist)
			{
				free(extnamelist);
				extnamelist = NULL;
			}
			
			btn->value(0);
			win->rb_ff->value(1);
		}
		else
		{
			char efnl[1024];
			
			snprintf(efnl, 1024, "workdir/%s", fc->value());
			
			extnamelist = (char*) malloc(strlen(efnl)+1);
			
			strcpy(extnamelist, efnl);
#ifdef DEBUG
			printf("%s\n", extnamelist);
#endif
		}
	}
}

void cb_run_btn(Fl_Button *btn, void* userdata)
{
	int ret;
	int mode = win->get_cur_mode();
	int fmode = win->btn_all_files->value();
	char source[256];
	
	snprintf(source, 256, "%s%s", ((const char *[]) {"infiles/", "workdir/"})[mode], 
								  !fmode ? file : "\0");
	
	/*const char *source = (!fmode) ? (!mode) ? 
											  strcpy((char *) "infiles/", file) : 
											  strcpy((char *) "workdir/", file) : 
									(!mode) ? "infiles/" : 
											  "workdir/";*/
	
	float max = !fmode ? 1.0f : (float) win->fbrowser1->size();
	
	snprintf(l_pbar1, sizeof(l_pbar1), "0/%d", (int) max);
#ifdef DEBUG
	printf("%s %s %s %d файлов\n", ((const char *[]) {"распаковать", "упаковать"})[mode],
								   ((const char *[]) {"файл", "папку"})[fmode], 
								   source, (int) max);
#endif
	pb->w_pbar1->minimum(0.0f);
	pb->w_pbar1->maximum(max);
	pb->w_pbar1->value(0.0f);
	pb->w_pbar1->label(l_pbar1);
	
	pb->w_pbar2->minimum(0.0f);
	pb->w_pbar2->maximum(1.0f);
	pb->w_pbar2->value(0.0f);
	pb->w_pbar2->label("0%");
	
	pb->show();
	Fl::check();
	
	for (int i = 0; i < 50; i++)
	{
		Fl::check();
		usleep(100000);
	}
	
	int (*func[4])(const char *) = { unpack_afs,  pack_afs, unpack_folder, pack_folder};
	
	ret = (*func[(fmode<<1)+mode])(source);
	
	if (!fmode)
	{
		pb->w_pbar1->label("1/1");
		pb->w_pbar1->value(max);
	}
	
	
	
	if (ret < 0 || (fmode && ret != (int) max))
	{
		fl_beep(FL_BEEP_ERROR);
		if (fl_ask("%s завершилась с ошибками\nЗакрыть программу?\n", 
				  ((const char *[]) {"Распаковка", "Упаковка"})[mode]))
				  exit(0);
	}
	else
	{
		fl_beep();
		if (fl_ask("%s завершилась удачно\nЗакрыть программу?\n", 
				  ((const char *[]) {"Распаковка", "Упаковка"})[mode]))
				  exit(0);
	}
	
	pb->hide();
}

AFSToolsWindow::AFSToolsWindow()
{
	cur_mode = 0;
	
	fbrowser1->callback((Fl_Callback *) cb_fsel1, this);
	fbrowser1->filetype(Fl_File_Browser::FILES);
	fbrowser1->filter("*.afs");
	fbrowser1->load("infiles/");
	
	btn_pack->callback((Fl_Callback *) cb_btn_pack_callback, this);
	btn_unpack->callback((Fl_Callback *) cb_btn_unpack_callback, this);
	
	btn_all_files->callback((Fl_Callback *) cb_conv_all, this);
	
	rb_ff->callback((Fl_Callback *) cb_name_sourse, this);
	rb_fl->callback((Fl_Callback *) cb_name_sourse, this);
	rb_nn->callback((Fl_Callback *) cb_name_sourse, this);
	
	fc = new OF_Choiser(m_window, "workdir", "*.lst");
	
	run_btn->callback((Fl_Callback *) cb_run_btn, this);
	
	pb = new Progress_Bar(m_window);
}

void AFSToolsWindow::show()
{
	m_window->show();
}


