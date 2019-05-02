CFTE_OBJS =\
 cfte.$(OEXT)\
 s_files.$(OEXT)\
 s_string.$(OEXT)

OBJS =\
 commands.$(OEXT)\
 c_bind.$(OEXT)\
 c_color.$(OEXT)\
 c_config.$(OEXT)\
 c_desktop.$(OEXT)\
 c_hilit.$(OEXT)\
 c_history.$(OEXT)\
 c_mode.$(OEXT)\
 egui.$(OEXT)\
 e_block.$(OEXT)\
 e_buffer.$(OEXT)\
 e_cmds.$(OEXT)\
 e_cvslog.$(OEXT)\
 e_file.$(OEXT)\
 e_fold.$(OEXT)\
 e_line.$(OEXT)\
 e_loadsave.$(OEXT)\
 e_mark.$(OEXT)\
 e_print.$(OEXT)\
 e_redraw.$(OEXT)\
 e_regex.$(OEXT)\
 e_search.$(OEXT)\
 e_svnlog.$(OEXT)\
 e_tags.$(OEXT)\
 e_trans.$(OEXT)\
 e_undo.$(OEXT)\
 fte.$(OEXT)\
 gui.$(OEXT)\
 g_draw.$(OEXT)\
 g_menu.$(OEXT)\
 h_ada.$(OEXT)\
 h_c.$(OEXT)\
 h_catbs.$(OEXT)\
 h_fte.$(OEXT)\
 h_ipf.$(OEXT)\
 h_make.$(OEXT)\
 h_msg.$(OEXT)\
 h_pascal.$(OEXT)\
 h_perl.$(OEXT)\
 h_plain.$(OEXT)\
 h_rexx.$(OEXT)\
 h_sh.$(OEXT)\
 h_simple.$(OEXT)\
 h_tex.$(OEXT)\
 indent.$(OEXT)\
 i_ascii.$(OEXT)\
 i_choice.$(OEXT)\
 i_complete.$(OEXT)\
 i_input.$(OEXT)\
 i_key.$(OEXT)\
 i_modelview.$(OEXT)\
 i_oview.$(OEXT)\
 i_search.$(OEXT)\
 i_view.$(OEXT)\
 log.$(OEXT)\
 o_buffer.$(OEXT)\
 o_buflist.$(OEXT)\
 o_cvs.$(OEXT)\
 o_cvsbase.$(OEXT)\
 o_cvsdiff.$(OEXT)\
 o_directory.$(OEXT)\
 o_list.$(OEXT)\
 o_messages.$(OEXT)\
 o_model.$(OEXT)\
 o_modemap.$(OEXT)\
 o_routine.$(OEXT)\
 o_svn.$(OEXT)\
 o_svnbase.$(OEXT)\
 o_svndiff.$(OEXT)\
 stl_string.$(OEXT)\
 s_direct.$(OEXT)\
 s_files.$(OEXT)\
 s_string.$(OEXT)\
 s_util.$(OEXT)\
 view.$(OEXT)

OS2OBJS =\
 e_os2.$(OEXT)

VIOOBJS = $(OS2OBJS)\
 clip_os2.$(OEXT)\
 clip_pmv.$(OEXT)\
 con_os2.$(OEXT)\
 g_nodlg.$(OEXT)\
 g_text.$(OEXT)\
 menu_text.$(OEXT)

PMOBJS = $(OS2OBJS)\
 clip_os2.$(OEXT)\
 clip_pm.$(OEXT)\
 g_pm.$(OEXT)

UNIXOBJS =\
 e_unix.$(OEXT)\
 memicmp.$(OEXT)

XOBJS = $(UNIXOBJS)\
 clip_x11.$(OEXT)\
 con_i18n.$(OEXT)\
 con_x11.$(OEXT)\
 g_nodlg.$(OEXT)\
 g_text.$(OEXT)\
 g_unix_pipe.$(OEXT)\
 menu_text.$(OEXT)

QOBJS = $(UNIXOBJS)\
 clip_x11.$(OEXT)\
 g_qt.$(OEXT)\
 g_qt_dlg.$(OEXT)

SOBJS = $(UNIXOBJS)\
 clip_no.$(OEXT)\
 con_slang.$(OEXT)\
 g_nodlg.$(OEXT)\
 g_text.$(OEXT)\
 g_unix_pipe.$(OEXT)\
 menu_text.$(OEXT)

NOBJS = $(UNIXOBJS)\
 clip_no.$(OEXT)\
 con_ncurses.$(OEXT)\
 g_nodlg.$(OEXT)\
 g_text.$(OEXT)\
 g_unix_pipe.$(OEXT)\
 menu_text.$(OEXT)

VOBJS = $(UNIXOBJS)\
 clip_no.$(OEXT)\
 con_linux.$(OEXT)\
 g_nodlg.$(OEXT)\
 g_text.$(OEXT)\
 g_unix_pipe.$(OEXT)\
 menu_text.$(OEXT)

MOBJS = $(UNIXOBJS)\
 clip_no.$(OEXT)\
 g_motif.$(OEXT)\
 g_nodlg.$(OEXT)

NTOBJS =\
 clip_os2.$(OEXT)\
 con_nt.$(OEXT)\
 e_win32.$(OEXT)\
 g_nodlg.$(OEXT)\
 g_text.$(OEXT)\
 menu_text.$(OEXT)

DOSP32OBJS =\
 clip_no.$(OEXT)\
 con_dosx.$(OEXT)\
 e_djgpp2.$(OEXT)\
 g_nodlg.$(OEXT)\
 g_text.$(OEXT)\
 memicmp.$(OEXT)\
 menu_text.$(OEXT)\
 port.$(OEXT)\
 portdos.$(OEXT)
