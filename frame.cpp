#pragma once

#include "frame.h"
#include "curlhelp.cpp"
#include "icon.xpm"
#include "locale.cpp"
#include "trevtdata.h"
#include <algorithm>
#include <curl/curl.h>
#include <set>
#include <thread>
#include <wx/aboutdlg.h>

wxDEFINE_EVENT(CLOSE_WINDOW, wxCommandEvent);
wxDEFINE_EVENT(UPDATE_APPLICATION, wxCommandEvent);
// wxDEFINE_EVENT(ENABLE_CONTROLS, wxCommandEvent);
// wxDEFINE_EVENT(DISABLE_CONTROLS, wxCommandEvent);
BEGIN_EVENT_TABLE(Frame, wxFrame)
EVT_COMMAND(wxID_ANY, CLOSE_WINDOW, Frame::destroyOnceDone)
EVT_COMMAND(wxID_ANY, UPDATE_APPLICATION, Frame::updateTest)
// EVT_COMMAND(wxID_ANY, ENABLE_CONTROLS, Frame::enableControls)
// EVT_COMMAND(wxID_ANY, DISABLE_CONTROLS, Frame::disableControls)
EVT_ICONIZE(Frame::hide)
EVT_CHAR_HOOK(Frame::onKey)
END_EVENT_TABLE()

Frame::Frame(wxString name, wxArrayString args)
    : wxFrame(NULL, wxID_ANY, name)
    , args(args) {
	SetMaxSize(wxSize(480, 480));
	SetMinSize(wxSize(480, 240));

	trayIcon     = new TrayIcon(this);
	readyMessage = READY;

	// define components
	panel                = new wxPanel(this);
	topSizer             = new wxBoxSizer(wxVERTICAL);
	addButtons           = new wxBoxSizer(wxHORIZONTAL);
	addFontFilesButton   = new wxButton(panel, wxID_ANY, wxString::FromUTF8(ADD_FILES));
	addFontFoldersButton = new wxButton(panel, wxID_ANY, wxString::FromUTF8(ADD_FOLDER));
	removeAllFontsButton = new wxButton(panel, wxID_ANY, wxString::FromUTF8(R_ALL));
	statuses             = new wxBoxSizer(wxHORIZONTAL);
	statusText1          = new wxStaticText(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                   wxST_ELLIPSIZE_END | wxST_NO_AUTORESIZE | wxALIGN_LEFT);
	statusText2 = new wxStaticText(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END | wxALIGN_RIGHT);
	statusText3 = new wxStaticText(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END | wxALIGN_RIGHT);
	removeSelectedButton = new wxButton(panel, wxID_ANY, wxString::FromUTF8(R));
	// recursiveSearch = new wxCheckBox(panel, wxID_ANY, wxString::FromUTF8(RECURSIVE));

	fontTree     = new wxTreeCtrl(panel, wxID_FILECTRL, wxDefaultPosition, wxDefaultSize,
                              wxTR_DEFAULT_STYLE | wxTR_HAS_BUTTONS | wxTR_TWIST_BUTTONS | wxTR_NO_LINES |
                                  wxTR_FULL_ROW_HIGHLIGHT | wxTR_HIDE_ROOT | wxTR_MULTIPLE);
	fontTreeRoot = fontTree->AddRoot(wxString::FromUTF8(FONTS));

	TreeItemData *folders = new TreeItemData(FOLDERS, true, false, LD_FONT_FOLDERS);
	fontTreeFolders       = fontTree->AppendItem(fontTreeRoot, wxString::FromUTF8(FOLDERS), -1, -1, folders);
	TreeItemData *files   = new TreeItemData(FILES, true, false, LD_FONT_FILES);
	fontTreeFiles         = fontTree->AppendItem(fontTreeRoot, wxString::FromUTF8(FILES), -1, -1, files);
	TreeItemData *errors  = new TreeItemData(ERRORS, true, true, E_ITEMS);
	fontTreeFailed        = fontTree->AppendItem(fontTreeRoot, wxString::FromUTF8(ERRORS), -1, -1, errors);

	// put components together
	topSizer->Add(addButtons, wxSizerFlags().Align(wxCENTRE).Border(wxALL, padding).Expand());
	addButtons->Add(addFontFilesButton, wxSizerFlags(1).Align(wxCENTRE).Border(wxRIGHT, padding));
	addButtons->Add(addFontFoldersButton, wxSizerFlags(0).Align(wxCENTRE).Border(wxRIGHT, padding));
	// addButtons->Add(recursiveSearch, wxSizerFlags(1).Align(wxCENTRE).Border(wxRIGHT, padding));
	addButtons->Add(removeSelectedButton, wxSizerFlags(0).Align(wxCENTRE).Border(wxRIGHT, padding));
	addButtons->Add(removeAllFontsButton, wxSizerFlags(0).Align(wxCENTRE));

	topSizer->Add(new wxStaticLine(panel, wxID_ANY, wxDefaultPosition, wxSize(0, 1)), wxSizerFlags().Align(wxCENTRE).Expand());
	topSizer->Add(fontTree, wxSizerFlags(1).Expand().Border(wxLEFT | wxRIGHT | wxTOP, padding));
	topSizer->Add(statuses, wxSizerFlags().Expand());

	statuses->Add(statusText1, wxSizerFlags(1).Expand().Border(wxLEFT | wxRIGHT, padding));
	statuses->Add(new wxStaticLine(panel, wxID_ANY, wxDefaultPosition, wxSize(1, 0)), wxSizerFlags().Align(wxRIGHT).Expand());
	statuses->Add(statusText3, wxSizerFlags(0).Border(wxLEFT | wxRIGHT, padding));
	errorLine = new wxStaticLine(panel, wxID_ANY, wxDefaultPosition, wxSize(1, 0));
	statuses->Add(errorLine, wxSizerFlags().Align(wxRIGHT).Expand());
	statuses->Add(statusText2, wxSizerFlags(0).Border(wxLEFT | wxRIGHT, padding));
	statuses->Show(errorLine, false);
	statuses->Show(statusText2, false);

	// theming
	// panel->SetBackgroundColour(*wxWHITE);

	// give functionality
	addFontFilesButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &Frame::addFontFilesFromDialog, this);
	addFontFoldersButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &Frame::addFontFoldersFromDialog, this);
	removeAllFontsButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &Frame::removeAllFontsWithButton, this);
	removeSelectedButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &Frame::removeSelected, this);
	removeSelectedButton->Disable();

	panel->Connect(wxEVT_DROP_FILES, wxDropFilesEventHandler(Frame::handleDroppedFiles), NULL, this);

	Bind(wxEVT_CLOSE_WINDOW, &Frame::onClose, this);

	// set styles
	// SetWindowStyle(wxDEFAULT_FRAME_STYLE & ~(wxMAXIMIZE_BOX));
	EnableMaximizeButton(false);

	// show stuff
	updateStatus();
	SetIcon(icon_xpm);
	panel->SetSizer(topSizer);
	Centre();
	Raise();
	SetFocus();
	Show();

	updateStatus(READING_FILESYSTEM "...");

	succeededFontFiles = {};
	failedFontFiles    = {};

	// open about dlg on first start
	wxCommandEvent evt;
	if (!boost::filesystem::is_regular_file(databasePath))
		showAbout(evt);

	// filesystem stuff
	savedFromFile.open(databasePath);
	savedFromFile.clear();
	savedFromFile.close();
	saveToFile.open(databasePath, std::ofstream::app);
	saveToFile.clear();
	saveToFile.flush();
	saveToFile.close();

	// if (!(GetFileAttributesA(databasePath.c_str()) & FILE_ATTRIBUTE_HIDDEN))
	// 	SetFileAttributesA(databasePath.c_str(), GetFileAttributesA(databasePath.c_str()) | FILE_ATTRIBUTE_HIDDEN);

	addFontsFromFile();
	std::thread(&Frame::waitForCompletion, this, false, true).detach();
}

Frame::~Frame() {
	delete trayIcon;
}

void Frame::waitForCompletion(bool reset, bool loadArgs) {
	std::unique_lock<std::mutex> lock = std::unique_lock<std::mutex>(m);
	cv.wait(lock, [&]() { return processCount == 0; });

	if (reset) {
		oldSucceededFontFiles = {};
		oldFailedFontFiles    = {};
		failedFontFiles       = {};
		folders               = {};
	}

	processRemoveQueue(succeededFontFiles);

	std::sort(succeededFontFiles.begin(), succeededFontFiles.end(),
	          [](const FontInfo &a, const FontInfo &b) -> bool { return a.fileName.compare(b.fileName) < 0; });
	std::sort(failedFontFiles.begin(), failedFontFiles.end(),
	          [](const FontInfo &a, const FontInfo &b) -> bool { return a.fileName.compare(b.fileName) < 0; });

	if (loadArgs && args.size() > 0) {
		addFontsFromArgs(args);
		return;
	} else {
		addToTree();
		oldFailedFontFiles    = failedFontFiles;
		oldSucceededFontFiles = succeededFontFiles;
	}

	if (changed) {
		changed = false;
		statuses->Layout();
		updateStatus(UPDATING_DATABASE "...");
		save();

		std::thread(SendMessageA, HWND_BROADCAST, WM_FONTCHANGE, NULL, NULL).detach();
	}

	enableControls();
}

void Frame::addFontFilesFromDialog(wxCommandEvent &evt) {
	evt.Skip();
	updateStatus(A_FONT_FILES "...");
	// open dialogue
	wxFileDialog fontFileDialog(
	    this, wxString::FromUTF8(A_FONT_FILES), "%APPDATA%", "",
	    wxString::FromUTF8(FONT_FILES
	                       " "
	                       "(*.fon;*.fnt;*.ttf;*.ttc;*.fot;*.otf;*.mmm;*.pfb;*.pfm)|*.fon;*.fnt;*.ttf;*.ttc;*.fot;*.otf;*."
	                       "mmm;*.pfb;*.pfm"),
	    wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
	if (fontFileDialog.ShowModal() == wxID_CANCEL) {
		Enable(); // reenable as window is disabled from the tray
		updateStatus(readyMessage);
		return;
	}

	disableControls();

	updateStatus(LG_FONTS "...");
	// get file paths
	wxArrayString a;
	fontFileDialog.GetPaths(a);

	for (auto &s : a) {
		std::string path = boost::filesystem::canonical(s.ToStdString()).string();
		wxPuts(path);
		addFontFileAsync(path);
	}

	Enable(); // reenable as window is disabled from the tray
	std::thread(&Frame::waitForCompletion, this, false, false).detach();
}

void Frame::addFontFileAsync(std::string path) {
	// loop through file paths and load them
	updateStatus(INDEXING "...");

	try {
		FontInfo currentFont(path);

		bool found = false;
		for (const FontInfo &s : succeededFontFiles) {
			if (s.path == currentFont.path) {
				found = true;
				break;
			}
		}

		if (!found) {
			processCount++;                                           // help for detecting when everything is done
			std::thread(&Frame::addFont, this, currentFont).detach(); // add font
		} else {
			updateStatus();
		}
	} catch (...) {
	}
}

void Frame::addFontsFromArgs(const wxArrayString &args) {
	for (auto i = args.begin() + 1; i != args.end(); ++i) {
		wxPuts(*i);
		try {
			std::string path = boost::filesystem::canonical(i->ToStdString()).string();

			if (boost::filesystem::is_directory(path))
				addFontFolderAsync(path, boost::filesystem::canonical(path).filename().string());
			else if (boost::filesystem::is_regular_file(path))
				addFontFileAsync(path);
		} catch (std::exception &e) {
			failedFontFiles.emplace_back(i->ToStdString());
		}
	}

	std::thread(&Frame::waitForCompletion, this, false, false).detach();
}

void Frame::addFontFromInfoAsync(FontInfo currentFont) {
	try {
		bool found = false;
		for (const FontInfo &s : succeededFontFiles) {
			if (s.path == currentFont.path) {
				found = true;
				break;
			}
		}
		if (!found) {
			processCount++;                                           // help for detecting when everything is done
			std::thread(&Frame::addFont, this, currentFont).detach(); // add font
		} else {
			updateStatus();
		}
	} catch (...) {
	}
}

void Frame::addFontFoldersFromDialog(wxCommandEvent &evt) {
	evt.Skip();
	updateStatus(A_FONT_FOLDERS);
	// open folder dialogue
	wxDirDialog fontDirDialog(this, wxString::FromUTF8(A_FONT_FOLDERS), "%APPDATA%", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
	if (fontDirDialog.ShowModal() == wxID_CANCEL) {
		Enable(); // reenable as window is disabled from the tray
		updateStatus(readyMessage);
		return;
	}

	disableControls();

	std::string dirPath    = boost::filesystem::canonical(fontDirDialog.GetPath().ToStdString()).string();
	std::string folderName = boost::filesystem::path(dirPath).filename().string();

	addFontFolderAsync(dirPath, folderName);

	Enable(); // reenable as window is disabled from the tray
	std::thread(&Frame::waitForCompletion, this, false, false).detach();
}

void Frame::addFontFolderAsync(std::string dirPath, std::string folderName) {
	tbb::concurrent_vector<FontInfo> fontsInFolder;
	// loop, index and load
	updateStatus(INDEXING "...");

	bool failedFolder = true;

	for (auto &p : boost::filesystem::directory_iterator(dirPath)) {
		if (boost::filesystem::is_regular_file(p) && isFontFile(p.path().string()))
			failedFolder = false,
			fontsInFolder.emplace_back(boost::filesystem::canonical(p.path()).string(), dirPath, folderName);
	}

	if (failedFolder) {
		failedFontFiles.emplace_back(dirPath, dirPath, folderName);
		return;
	}

	tbb::concurrent_vector<FontInfo> tempFonts;
	for (const FontInfo &t : fontsInFolder) {
		bool found = false;
		for (FontInfo &l : succeededFontFiles) {
			if (t.path == l.path && t.folderPath != l.folderPath) {
				found        = true;
				l.folderPath = t.folderPath;
				l.fileName   = t.fileName;
				l.folder     = t.folder;
			}

			for (TreeItemData *&i : ((TreeItemData *) fontTree->GetItemData(fontTreeFiles))->children) {
				if (i && i->GetId().IsOk() && i->path == l.path) {
					wxTreeItemId id = i->GetId();
					i               = 0;
					fontTree->UnselectItem(id);
					fontTree->Delete(id);
				}
			}
		}

		if (!found) {
			tempFonts.push_back(t);
		} else {
			changed = true;
		}
	}
	for (FontInfo &f : tempFonts) {
		try {
			wxPuts(f.path);
			bool found = false;
			for (const FontInfo &s : succeededFontFiles) {
				if (s.path == f.path) {
					found = true;
					break;
				}
			}

			if (!found) {
				processCount++;
				std::thread(&Frame::addFont, this, f).detach();
			} else {
				updateStatus();
			}
		} catch (...) {
		}
	}
}

bool Frame::addFont(FontInfo &font) {
	bool success;
	try {
#if LANG == 0
		updateStatus(LG ": " + font.fileName);
#elif LANG == 1
		updateStatus(font.fileName + " " LG);
#endif // LANG == 0
		success = AddFontResourceA(font.path.c_str());

		if (success) {
			succeededFontFiles.push_back(font);

#if LANG == 0
			updateStatus(LD ": " + font.fileName);
#elif LANG == 1
			updateStatus(font.fileName + " " LD);
#endif // LANG == 0

			changed = true;
		} else
			failedFontFiles.push_back(font), updateStatus();

		updateStatus();
		std::lock_guard<std::mutex> guard(m);
		processCount--;
		cv.notify_all();
	} catch (...) {
		success = false;
	}
	return success;
}

void Frame::removeAllFontsWithButton(wxCommandEvent &evt) {
	evt.Skip();
	disableControls();

	std::ofstream clear(databasePath, std::ofstream::out | std::ofstream::trunc);
	clear.clear();
	clear.flush();
	clear.close();

	handleTreeItem(fontTreeFolders);
	handleTreeItem(fontTreeFiles);
	handleTreeItem(fontTreeFailed);

	std::thread(&Frame::waitForCompletion, this, false, false).detach();
}

bool Frame::removeFont(FontInfo &font) {
	bool success;

	try {
		bool temp;
		int  count = 0;
#if LANG == 0
		updateStatus(ULG ": " + font.fileName);
#elif LANG == 1
		updateStatus(font.fileName + " " ULG);
#endif // LANG == 0
		while ((temp = RemoveFontResourceA(font.path.c_str()))) {
			if (count == 0)
				success = temp;
			count++;
		}
		if (success) {
			addToRemoveQueue(succeededFontFiles, font);

#if LANG == 0
			updateStatus(ULD ": " + font.fileName);
#elif LANG == 1
			updateStatus(font.fileName + " " ULD);
#endif // LANG == 0

			changed = true;
		}
		updateStatus();
		std::lock_guard<std::mutex> guard(m);
		processCount--;
		cv.notify_all();
	} catch (...) {
		success = false;
	}

	return success;
}

template <class T>
bool Frame::addToRemoveQueue(tbb::concurrent_vector<T> &list, T item) {
	bool found = false;
	for (unsigned long long i = 0; i < list.size(); i++) {
		if (list[i] == item) {
			found = true;
			// list.erase(list.begin() + i);
			removeList.push_back(i);
		}
	}

	return found;
}

template <class T>
void Frame::processRemoveQueue(tbb::concurrent_vector<T> &list) {
	tbb::concurrent_vector<T> tempList;
	for (unsigned long long i = 0; i < list.size(); i++)
		if (!isInVector(removeList, i))
			tempList.push_back(list[i]);

	list       = tempList;
	removeList = {};
}

template <class T>
bool Frame::isInVector(tbb::concurrent_vector<T> &list, T item) {
	for (unsigned long long i = 0; i < list.size(); i++)
		if (list[i] == item)
			return true;

	return false;
}

bool Frame::isFontFile(boost::filesystem::path path) {
	tbb::concurrent_vector<std::string> extensions = {".fon", ".fnt", ".ttf", ".ttc", ".fot", ".otf", ".mmm", ".pfb", ".pfm"};
	return isInVector(extensions, path.extension().string());
}

void Frame::addToTree() {
	fontTree->UnselectAll();
	// fontTree->CollapseAll();

	for (FontInfo &f : succeededFontFiles) {
		if (!isInVector(oldSucceededFontFiles, f))
			if (!f.folder.empty()) {
				// if font belongs to a folder
				fontTree->Expand(fontTreeFolders);

				// keep track of whether folder is already made in the tree
				bool folderExists = false;
				for (TreeItem &t : folders) {
					// if found an existing folder
					if (t.name == f.folder) {
						// create a new tree item
						TreeItemData *file = new TreeItemData(f.path, false, false, f.folderPath);
						// add item to folder
						fontTree->AppendItem(t.id, f.fileName, -1, -1, file), folderExists = true; // NEW
						((TreeItemData *) fontTree->GetItemData(t.id))->appendChild(file);
						fontTree->SelectItem(t.id);
						// fontTree->Expand(t.id);
					}
				}

				if (!folderExists) {
					TreeItemData *folder = new TreeItemData(f.folder, true, false, f.folderPath);
					folders.emplace_back(fontTree->AppendItem(fontTreeFolders, f.folder, -1, -1, folder),
					                     f.folder); // NEW FOLDER
					((TreeItemData *) fontTree->GetItemData(fontTreeFolders))->appendChild(folder);
					TreeItemData *file = new TreeItemData(f.path, false, false, f.folderPath);
					fontTree->AppendItem((folders.end() - 1)->id, f.fileName, -1, -1, file); // NEW
					((TreeItemData *) fontTree->GetItemData((folders.end() - 1)->id))->appendChild(file);
					fontTree->SelectItem((folders.end() - 1)->id);
					// fontTree->Expand((folders.end() - 1)->id);
				}
			} else {
				TreeItemData *file = new TreeItemData(f.path, false, false);
				fontTree->SelectItem(fontTree->AppendItem(fontTreeFiles, f.fileName, -1, -1, file)); // NEW
				((TreeItemData *) fontTree->GetItemData(fontTreeFiles))->appendChild(file);
				fontTree->Expand(fontTreeFiles);
			}
	}

	for (FontInfo &f : failedFontFiles) {
		bool found = false;
		for (const FontInfo &o : oldFailedFontFiles) {
			if (f.path == o.path) {
				found = true;
				break;
			}
		}
		if (!found) {
			TreeItemData *errorFile = new TreeItemData(f.path, false, true);
			fontTree->SelectItem(fontTree->AppendItem(fontTreeFailed, f.fileName, -1, -1, errorFile)); // NEW
			fontTree->Expand(fontTreeFailed);

			((TreeItemData *) fontTree->GetItemData(fontTreeFailed))->appendChild(errorFile);
		}
	}

	tbb::concurrent_vector<TreeItem> newFolders;
	for (int i = 0; i < folders.size(); i++) {
		if (fontTree->GetChildrenCount(folders[i].id) == 0) {
			fontTree->UnselectItem(folders[i].id);
			fontTree->Delete(folders[i].id);
		} else {
			newFolders.push_back(folders[i]);
		}
	}

	folders = newFolders;
}

void Frame::closeOnceDone() {
	std::unique_lock<std::mutex> lock = std::unique_lock<std::mutex>(m);
	cv.wait(lock, [&]() { return processCount == 0; });

	std::thread(SendMessageA, HWND_BROADCAST, WM_FONTCHANGE, NULL, NULL).detach();
	updateStatus(ABOUT_TO_CLOSE);

	trayIcon->RemoveIcon();
	wxPostEvent(this, wxCommandEvent(CLOSE_WINDOW));
}

void Frame::onClose(wxCloseEvent &evt) {
	// evt.Skip();

	if (!busy) {
		evt.StopPropagation();
		disableControls();

		handleTreeItem(fontTreeFolders, false);
		handleTreeItem(fontTreeFiles, false);
		handleTreeItem(fontTreeFailed, false);

		std::thread(&Frame::closeOnceDone, this).detach();
	}
}

void Frame::updateStatus() {
	if (errorHidden && !failedFontFiles.empty()) {
		errorHidden = false;
		statuses->Show(errorLine, true);
		statuses->Show(statusText2, true);
		statuses->Layout();
		statuses->RecalcSizes();
		Refresh();
		wxPuts("Shown");
	}

	if (!errorHidden && failedFontFiles.empty()) {
		errorHidden = true;
		statuses->Show(errorLine, false);
		statuses->Show(statusText2, false);
		statuses->Layout();
		statuses->RecalcSizes();
		Refresh();
		wxPuts("Hidden");
	}

	statusText1->SetLabelText(wxString::FromUTF8(status.c_str()));
	statusText2->SetLabelText(wxString::FromUTF8(ERRORS ": ") + wxString(std::to_string(failedFontFiles.size()).c_str()));
	statusText3->SetLabelText(
	    wxString::FromUTF8(FONTS ": ") +
	    wxString(std::to_string(succeededFontFiles.size() -
	                            (removeList.size() > succeededFontFiles.size() ? succeededFontFiles.size() : removeList.size()))
	                 .c_str()));

	if (oldSuccessfulSize != succeededFontFiles.size() - removeList.size() || oldFailedSize != failedFontFiles.size()) {
		if (std::to_string(oldSuccessfulSize).length() !=
		        std::to_string(succeededFontFiles.size() - removeList.size()).length() ||
		    std::to_string(oldFailedSize).length() != std::to_string(failedFontFiles.size()).length())
			statuses->Layout(), wxPuts("Layout!");
	}

	oldSuccessfulSize = succeededFontFiles.size() - removeList.size();
	oldFailedSize     = failedFontFiles.size();
}

void Frame::updateStatus(std::string newStatus) {
	status = newStatus;
	updateStatus();
}

void Frame::removeSelected(wxCommandEvent &evt) {
	evt.Skip();

	wxArrayTreeItemIds ids;
	fontTree->GetSelections(ids);
	bool shouldRemove = false;

	for (const wxTreeItemId id : ids)
		if (fontTree->GetChildrenCount(id) > 0 || !((TreeItemData *) fontTree->GetItemData(id))->folder)
			shouldRemove = true;

	if (shouldRemove) {
		disableControls();
		for (wxTreeItemId id : ids) {
			handleTreeItem(id);
		}
		std::thread(&Frame::waitForCompletion, this, false, false).detach();
	}
}

void Frame::handleTreeItem(wxTreeItemId id, bool removeFromTreeToo) {
	try {
		if (id.IsOk()) {
			TreeItemData *tritemdata = (TreeItemData *) fontTree->GetItemData(id);
			if (tritemdata) {
				if (tritemdata->folder) {
					tbb::concurrent_vector<TreeItem> tempFolders;
					for (TreeItem i : folders) {
						if (i.id != id)
							tempFolders.push_back(i);
					}
					folders = tempFolders;

					for (TreeItemData *&child : tritemdata->children) {
						if (child) {
							wxTreeItemId id = child->GetId();
							child           = 0;
							handleTreeItem(id, removeFromTreeToo);
						}
					}

					if (tritemdata->path != FOLDERS && tritemdata->path != FILES && tritemdata->path != ERRORS &&
					    removeFromTreeToo)
						fontTree->UnselectItem(id), fontTree->Delete(id);
				} else if (!tritemdata->folder && !tritemdata->error) {
					oldSucceededFontFiles = succeededFontFiles;
					processCount++;
					std::thread(&Frame::removeFont, this,
					            FontInfo(tritemdata->path, tritemdata->folderPath, tritemdata->folderPath))
					    .detach();
					if (removeFromTreeToo)
						fontTree->UnselectItem(id), fontTree->Delete(id);
				} else if (!tritemdata->folder && tritemdata->error) {
					tbb::concurrent_vector<FontInfo> tempFailedFontFiles;
					for (FontInfo f : failedFontFiles) {
						if (f.path != tritemdata->path)
							tempFailedFontFiles.push_back(f);
					}

					failedFontFiles = tempFailedFontFiles;
					if (removeFromTreeToo)
						fontTree->UnselectItem(id), fontTree->Delete(id);
				}
			}
		}
	} catch (...) {
	}
}

void Frame::save() {
	saveToFile.open(databasePath, std::iostream::out | std::iostream::trunc);
	saveToFile.clear();

	for (FontInfo &f : succeededFontFiles) {
		std::string path       = boost::filesystem::relative(f.path).string();
		std::string folderPath = boost::filesystem::relative(f.folderPath).string();

		if (path.empty())
			path = boost::filesystem::canonical(f.path).string();
		if (folderPath.empty())
			folderPath = boost::filesystem::canonical(f.folderPath).string();

		saveToFile << path << "\t" << (f.folderPath.empty() ? "" : folderPath).c_str() << "\n";
		saveToFile.flush();
	}

	saveToFile.clear();
	saveToFile.flush();
	saveToFile.close();
}

void Frame::addFontsFromFile() {
	disableControls();

	savedFromFile.open(databasePath);
	savedFromFile.clear();
	std::set<std::string> paths;
	std::string           item;
	while (getline(savedFromFile, item))
		paths.insert(item);

	for (const std::string &i : paths) {
		std::string path       = i.substr(0, i.find_first_of('\t'));
		std::string folderPath = i.substr(i.find_first_of('\t') + 1);
		if (boost::filesystem::is_regular_file(path)) {
			std::string pathAbsolute       = boost::filesystem::canonical(path).string();
			std::string folderPathAbsolute = boost::filesystem::canonical(folderPath).string();
			std::string folderName         = boost::filesystem::canonical(folderPath).filename().string();
			FontInfo    currentFont(pathAbsolute, (folderPath.empty() ? "" : folderPathAbsolute),
                                 (folderPath.empty() ? "" : folderName));
			addFontFromInfoAsync(currentFont);
		} else {
			failedFontFiles.emplace_back(path);
			updateStatus();
		}
	}

	savedFromFile.clear();
	savedFromFile.close();
}

void Frame::handleDroppedFiles(wxDropFilesEvent &evt) {
	evt.Skip();

	if (evt.GetNumberOfFiles() > 0) {
		disableControls();

		wxString *droppedFiles = evt.GetFiles();
		wxASSERT(droppedFiles);
		for (unsigned long long i = 0; i < evt.GetNumberOfFiles(); i++) {
			wxPuts(droppedFiles[i]);
			std::string path = boost::filesystem::canonical(droppedFiles[i].ToStdString()).string();

			if (boost::filesystem::is_directory(path))
				addFontFolderAsync(path, boost::filesystem::canonical(path).filename().string());
			else if (boost::filesystem::is_regular_file(path))
				addFontFileAsync(path);
		}

		std::thread(&Frame::waitForCompletion, this, false, false).detach();
	}
}

void Frame::handleSelectionChanged(wxTreeEvent &evt) {
	evt.Skip();

	if (!busy) {
		if (shouldEnableRemove())
			removeSelectedButton->Enable();
		else
			removeSelectedButton->Disable();

		wxPuts("SELECTION CHANGED");
	}
}

void Frame::hide(wxIconizeEvent &evt) {
	Show(false);
}

void Frame::treePopupMenu(wxMouseEvent &evt) {
	evt.Skip();

	wxTreeItemId id = fontTree->HitTest(evt.GetPosition());
	if (id && id.m_pItem && id.IsOk()) {
		bool               isAlreadySelected = false;
		wxArrayTreeItemIds ids;
		fontTree->GetSelections(ids);

		for (wxTreeItemId i : ids) {
			if (i == id) {
				isAlreadySelected = true;
				break;
			}
		}

		if (!isAlreadySelected) {
			fontTree->UnselectAll();
			fontTree->SelectItem(id);
		}

		fontTree->GetSelections(ids);

		wxMenu *menu = new wxMenu;

		if (shouldEnableRemove() && ids.size() > 1)
			menu->Append(wxID_REMOVE, wxString::FromUTF8(R_SELECTIONS));
		else if (id != fontTreeFailed && id != fontTreeFiles && id != fontTreeFolders) {
			if (fontTree->GetChildrenCount(id) > 0 && ids.size() == 1) {
				menu->Append(wxID_REMOVE, wxString::FromUTF8(R_FOLDER));
			} else if (ids.size() == 1 && !((TreeItemData *) fontTree->GetItemData(id))->error) {
				menu->Append(wxID_REMOVE, wxString::FromUTF8(R_FONT));
			} else {
				menu->Append(wxID_REMOVE, wxString::FromUTF8(R_E));
			}
		} else if (fontTree->GetChildrenCount(id) > 0) {
			std::string desc;
			if (id == fontTreeFailed)
				desc = R_A_E;
			else if (id == fontTreeFiles)
				desc = R_A_Files;
			else
				desc = R_A_Folders;
			menu->Append(wxID_REMOVE, wxString::FromUTF8(desc.c_str()));
		}
		Connect(wxID_REMOVE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Frame::removeSelected));

		Connect(wxID_REMOVE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Frame::removeSelected));
		if (id != fontTreeFailed && id != fontTreeFiles && id != fontTreeFolders) {
			TreeEventData *data = new TreeEventData(evt);
			menu->Append(wxID_OPEN, wxString::FromUTF8(OPEN));
			Connect(wxID_OPEN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(Frame::openSelected), data);
		}

		fontTree->PopupMenu(menu);
	}
}

void Frame::openSelected(wxCommandEvent &evt) {
	evt.Skip();

	wxTreeItemId id = fontTree->HitTest(((TreeEventData *) evt.GetEventUserData())->msEvent.GetPosition());
	fontTree->UnselectAll();
	fontTree->SelectItem(id);

	TreeItemData *data = ((TreeItemData *) fontTree->GetItemData(id));
	std::string   path;
	if (data->folder)
		path = data->folderPath;
	else
		path = data->path;
	std::replace(path.begin(), path.end(), '/', '\\');

	ShellExecuteA(GetHandle(), NULL, path.c_str(), NULL, NULL, SW_NORMAL);
}

void Frame::treeActivated(wxTreeEvent &evt) {
	evt.Skip();

	wxTreeItemId id = evt.GetItem();
	if (id.IsOk() && id.m_pItem && id != fontTreeFailed && id != fontTreeFiles && id != fontTreeFolders) {
		TreeItemData *data = ((TreeItemData *) fontTree->GetItemData(id));
		std::string   path;
		if (data->folder)
			path = data->folderPath;
		else
			path = data->path;
		std::replace(path.begin(), path.end(), '/', '\\');

		wxPuts(path);

		ShellExecuteA(GetHandle(), NULL, path.c_str(), NULL, NULL, SW_NORMAL);
	} else {
		wxPuts("failed");
		if (id.IsOk() && id.m_pItem) {
			fontTree->Toggle(id);
		}
	}
}

void Frame::showAbout(wxCommandEvent &evt) {
	evt.Skip();

	wxAboutDialogInfo info;
	info.SetName(wxString::FromUTF8(DESC_NAME));
	info.SetVersion(wxString::FromUTF8(VERSION));
	info.SetDescription(wxString::FromUTF8(DESC_DESC));
	info.SetCopyright(wxString::FromUTF8(DESC_COPYRIGHT));
	Disable();
	wxAboutBox(info, this);
	Enable();
}

void Frame::treeMotion(wxMouseEvent &evt) {
	evt.Skip();

	if (!fontTree->HitTest(evt.GetPosition()).IsOk())
		fontTree->SetToolTip("");
}

void Frame::disableControls() {
	wxPuts("!" + wxString::FromUTF8(READY));
	fontTree->Unbind(wxEVT_COMMAND_TREE_SEL_CHANGED, &Frame::handleSelectionChanged, this);
	fontTree->Unbind(wxEVT_RIGHT_DOWN, &Frame::treePopupMenu, this);
	fontTree->Unbind(wxEVT_COMMAND_TREE_ITEM_ACTIVATED, &Frame::treeActivated, this);
	fontTree->Unbind(wxEVT_TREE_ITEM_GETTOOLTIP, &Frame::getTreeItemTooltip, this);
	fontTree->Unbind(wxEVT_MOTION, &Frame::treeMotion, this);

	addFontFilesButton->Disable();
	removeAllFontsButton->Disable();
	addFontFoldersButton->Disable();
	removeSelectedButton->Disable();
	panel->DragAcceptFiles(false);

	EnableCloseButton(false);
	busy = true;
}

void Frame::enableControls() {
	if (shouldEnableRemove())
		removeSelectedButton->Enable();
	else
		removeSelectedButton->Disable();

	EnableCloseButton(true);
	busy = false;
	addFontFilesButton->Enable();
	addFontFoldersButton->Enable();

	if (succeededFontFiles.empty() && failedFontFiles.empty())
		removeAllFontsButton->Disable();
	else
		removeAllFontsButton->Enable();

	fontTree->Bind(wxEVT_RIGHT_DOWN, &Frame::treePopupMenu, this);
	fontTree->Bind(wxEVT_COMMAND_TREE_ITEM_ACTIVATED, &Frame::treeActivated, this);
	fontTree->Bind(wxEVT_COMMAND_TREE_SEL_CHANGED, &Frame::handleSelectionChanged, this);
	fontTree->Bind(wxEVT_TREE_ITEM_GETTOOLTIP, &Frame::getTreeItemTooltip, this);
	fontTree->Bind(wxEVT_MOTION, &Frame::treeMotion, this);
	panel->DragAcceptFiles(true);

	updateStatus(readyMessage);
	wxPuts(wxString::FromUTF8(READY));
}

void Frame::getTreeItemTooltip(wxTreeEvent &evt) {
	evt.Skip();

	std::string toolTip = ((TreeItemData *) fontTree->GetItemData(evt.GetItem()))->folder
	                          ? ((TreeItemData *) fontTree->GetItemData(evt.GetItem()))->folderPath + " (" +
	                                std::to_string(fontTree->GetChildrenCount(evt.GetItem(), false)) + ")"
	                          : ((TreeItemData *) fontTree->GetItemData(evt.GetItem()))->path;

	std::replace(toolTip.begin(), toolTip.end(), '/', '\\');
	evt.SetToolTip(toolTip);
}

void Frame::onKey(wxKeyEvent &evt) {
	evt.Skip();

	if ((evt.GetKeyCode() == 8 || evt.GetKeyCode() == 127) && !busy) {
		wxCommandEvent evt;
		removeSelected(evt);
		return;
	}
}

bool Frame::shouldEnableRemove() {
	wxArrayTreeItemIds ids;
	fontTree->GetSelections(ids);
	bool shouldEnable = false;

	for (const wxTreeItemId id : ids)
		if (fontTree->GetChildrenCount(id) > 0 || !((TreeItemData *) fontTree->GetItemData(id))->folder)
			shouldEnable = true;
	return shouldEnable;
}

void Frame::destroyOnceDone(wxCommandEvent &evt) {
	evt.Skip();

	wxPuts(wxString::FromUTF8(WILL_IT_DESTROY));
	Destroy();
	Destroy();
	Destroy();
	Destroy();
	Destroy();
}

void Frame::updateTest(wxCommandEvent &evt) {
	evt.Skip();
	disableControls();

	std::thread([&]() {
		// get latest version number
		updateStatus(CHECKING_UPDATES "...");

		CURLcode    code;
		std::string str(CURLHelper::GetUrlStringContents(
		    "https://raw.githubusercontent.com/fiercedeity-productions/font-tool/master/current-version", &code));

		if (code != CURLE_OK) {
			enableControls();
			updateStatus(E_UPDATE_INFO);
			std::this_thread::sleep_for(std::chrono::seconds(5));
			if (!busy)
				updateStatus(readyMessage);
			return;
		}

		if (str.compare(INTERNAL_VERSION) != 0) {
#if LANG == 0
			updateStatus(DOWNLOADING_VERSION " " + str + "...");
#elif LANG == 1
			updateStatus(VERSION_STR " " + str + " " DOWNLOADING "...");
#endif // LANG == 0

			CURLHelper::SaveUrlContents(
			    "https://raw.githubusercontent.com/fiercedeity-productions/font-tool/master/bin/" SERVER_FILE_NAME,
			    "new-" SERVER_FILE_NAME, &code);

			if (code != CURLE_OK) {
				enableControls();
				updateStatus(E_UPDATE_DOWNLOAD);
				std::this_thread::sleep_for(std::chrono::seconds(5));
				if (!busy)
					updateStatus(readyMessage);
				return;
			}

			boost::filesystem::rename(args.begin()->ToStdString(), args.begin()->ToStdString() + ".old");
			boost::filesystem::rename("new-" SERVER_FILE_NAME,
			                          boost::filesystem::path(args.begin()->ToStdString()).filename().string());

			updatePending = true;
			enableControls();
			updateStatus(UPDATE_READY);
			readyMessage = MESSAGE_UPDATED;

			std::this_thread::sleep_for(std::chrono::seconds(2));
			if (!busy)
				updateStatus(readyMessage);
		} else {
			enableControls();
			updateStatus(UP_TO_DATE);
			std::this_thread::sleep_for(std::chrono::seconds(2));
			if (!busy)
				updateStatus(readyMessage);
			return;
		}
	})
	    .detach();
}