/*

	ARIADNE
	Web-Based Escape Room Helper


*/

// standard template library
#include <iostream>
#include <vector>
#include <print>
#include <fstream>
#include <filesystem>

// libraries
#include <raylib.h>
#include <rlImGui.h>
#include <imgui.h>
#include <pugixml.hpp>
#include <misc/cpp/imgui_stdlib.h>

struct Step {
	std::string name;
	std::string url;
	std::string description;
	std::string username, password;
	
	std::vector<Step> children;
};


std::string decodeString(std::string str) {
	std::string out = "";
	const size_t size = str.size();
	for (int i = 0; i < size; i++) {
		if (str[i] == '\\' && i != size - 1) {
			if (str[i + 1] == 'n') {
				out += '\n';
			}
			else if (str[i + 1] == 'q') {
				out += '"';
			}
			else {
				out += str[i + 1];
			}
			i++;
		}
		else {
			out += str[i];
		}
	}
	return out;
}

std::string encodeString(std::string str) {
	std::string out = "";

	for (const char c : str) {
		if (c == '\n') {
			out += "\\n";
		}
		else if (c == '"') {
			out += "\\q";
		}
		else if(c == '\\') {
			out += "\\\\";
		}
		else {
			out += c;
		}
	}
	return out;
}


Step parseStep(pugi::xml_node step) {

	Step thisStep;

	thisStep.name = decodeString(step.attribute("name").as_string());
	thisStep.url = decodeString(step.attribute("url").as_string());
	thisStep.description = decodeString(
		step.attribute("description").as_string()
	);
	thisStep.username = decodeString(step.attribute("username").as_string());
	thisStep.password = decodeString(step.attribute("password").as_string());





	for (pugi::xml_node s : step.children("step")) {
		thisStep.children.push_back(parseStep(s));
	}
	return thisStep;

}

void loadIntoRoot(Step* root, std::string path) {

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(path.c_str());

	if (!result) throw std::out_of_range("file not xml or doesn't exist");
	
	*root = parseStep(doc.child("step"));

}

std::string encodeStep(Step step) {

	std::string encoded = std::format(
		"<step name=\"{}\" description=\"{}\" url=\"{}\" username=\"{}\" password=\"{}\">",
		encodeString(step.name),
		encodeString(step.description),
		encodeString(step.url),
		encodeString(step.username),
		encodeString(step.password)
	);

	for (Step child : step.children) {
		encoded += encodeStep(child);
	}
	encoded += "</step>";

	return encoded;
}

void saveFromRoot(Step* root, std::string path) {
	std::ofstream out(path);
	out << encodeStep(*root);
	out.close();
}

int main(int argc, char** argv) {
	
	Step root;
	root.name = "Test";
	root.url = "https://example.com";
	root.description = "Test description";
	root.username = "";
	root.password = "";
	loadIntoRoot(&root, "default.xml");

	std::string path = "default.xml";
	std::string previousPath = path;

	std::vector<int> objectPath;

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(1280, 720, "Ariadne");
	SetTargetFPS(60);
	SetExitKey(0);


	
	rlImGuiSetup(true);
	ImGui::GetIO().IniFilename = NULL;
	
	bool initializedListWindow = false;
	bool initializedEditWindow = false;
	
	bool showOpenDialog = false;
	bool initializedOpenDialog = false;
	bool showSaveDialog = false;
	bool initializedSaveDialog = false;
	
	bool badFileError = false;

	bool savingToDefaultError = false;
	bool savedSuccessfully = false;

	int selectedChild = -1;

	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(BLACK);

		Step* currentStep = &root;
		// traverse path
		for (int i = 0; i < objectPath.size(); i++) {
			currentStep = &(currentStep->children.at(objectPath[i]));
		}
		
		int height = 0;
		for (const Step& step : currentStep->children) {
			DrawText(step.name.c_str(), 0, height, 20, WHITE);
			height += 20;
		}



		rlImGuiBegin();

		
		if (!initializedListWindow) {
			ImGui::SetNextWindowPos({ 0,0 });
			ImGui::SetNextWindowSize(
				{(float)GetScreenWidth()/2,(float)GetScreenHeight()}
			);
			initializedListWindow = true;
		}

		// list window
		ImGui::Begin("List");

		if (ImGui::ArrowButton("Back", ImGuiDir_Left)) {
			if (objectPath.size() != 0) {
				
				selectedChild = objectPath.at(objectPath.size() - 1);
				objectPath.pop_back();
				
			}
		}
		ImGui::SameLine();
		if (ImGui::ArrowButton("Go", ImGuiDir_Right)) {
			if (selectedChild != -1) {
				objectPath.push_back(selectedChild);
			}
			selectedChild = -1;
		}
		ImGui::SameLine();
		if (ImGui::Button("Add")) {
			Step newStep;
			newStep.name = "New step";
			newStep.description = "";
			newStep.url = "";
			newStep.username = "";
			newStep.password = "";
			currentStep->children.push_back(newStep);
		}
		ImGui::SameLine();
		if (ImGui::Button("Delete")) {
			if (selectedChild != -1 &&
				selectedChild <= currentStep->children.size()) {
				if (currentStep->children[selectedChild].children.size() == 0) {
					currentStep->children.erase(
						currentStep->children.begin() + selectedChild
					);
					selectedChild = selectedChild < 
						currentStep->children.size() ?
						selectedChild : currentStep->children.size() - 1;
				}

				
			}
		}
		
		ImGui::Text("Current step: %s", currentStep->name.c_str());
		
		if (ImGui::BeginListBox("Steps", {0, ImGui::GetWindowHeight() - 100})) {
			for (int child = 0; child < currentStep->children.size(); child++) {
				Step& step = currentStep->children[child];
				bool selected = child == selectedChild;
				if (ImGui::Selectable(step.name.c_str(), &selected)) {
					selectedChild = child;
				}
			}
			ImGui::EndListBox();
		}
		ImGui::End();


		// edit window
		if (!initializedEditWindow) {
			ImGui::SetNextWindowPos({ (float)GetScreenWidth()/2,0});
			ImGui::SetNextWindowSize(
				{(float)GetScreenWidth() / 2, (float)GetScreenHeight()}
			);
			initializedEditWindow = true;
		}

		ImGui::Begin("Edit");
		
		Step* examined = currentStep;

		if (selectedChild != -1) {
			examined = &(examined->children.at(selectedChild));
		}

		
		ImGui::InputText("Name", &(examined->name));
		ImGui::InputTextMultiline(
			"Description", 
			&(examined->description), 
			{0, ImGui::GetWindowHeight() - 150}
		);
		ImGui::InputText("URL", &(examined->url));
		ImGui::InputText("Username", &(examined->username));
		ImGui::InputText("Password", &(examined->password));

		if (ImGui::Button("Go to page")) {
			
			std::string url = examined->url;
			if (examined->username != "" || examined->password != "") {
				bool secure = false;
				if (url.starts_with("https://")) {
					url.erase(0, 8);
					secure = true;
				}
				if (url.starts_with("http://")) {
					url.erase(0, 7);
					secure = false;
				}
				url = std::format(
					"{}://{}:{}@{}",
					secure ? "https" : "http",
					examined->username,
					examined->password,
					url
				);

			}
			OpenURL(url.c_str());
		}

		ImGui::End();



		if (IsKeyDown(KEY_LEFT_CONTROL)) {
			if (IsKeyPressed(KEY_O)) { // open
				initializedOpenDialog = false;
				showOpenDialog = true;
			}
			if (IsKeyPressed(KEY_S)) { // open
				initializedSaveDialog = false;
				showSaveDialog = true;
			}
		}

		if (showOpenDialog) {
			if (!initializedOpenDialog) {
				ImGui::SetNextWindowPos({ 100, 100 });
				ImGui::SetNextWindowSize({ 350, 200 });
				initializedOpenDialog = true;
				badFileError = false;
				previousPath = path;
			}
			ImGui::Begin("Open file");
			
			ImGui::Text("Opening a file will not save the current file.");

			ImGui::InputText("File path", &path);

			if (ImGui::Button("Load")) {
				try {
					loadIntoRoot(&root, path);
					previousPath = path;
					showOpenDialog = false;
				}
				catch (...) {
					badFileError = true;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel")) {
				path = previousPath;
				showOpenDialog = false;
			}

			if (badFileError)
				ImGui::Text("File not found or not an XML file.");
			

			ImGui::End();
		}

		if (showSaveDialog) {
			if (!initializedSaveDialog) {
				ImGui::SetNextWindowPos({ 100, 100 });
				ImGui::SetNextWindowSize({ 350, 200 });
				initializedSaveDialog = true;
				previousPath = path;
				savingToDefaultError = false;
				savedSuccessfully = false;
			}
			ImGui::Begin("Save file");


			ImGui::InputText("File path", &path);

			if (ImGui::Button("Save")) {
				savingToDefaultError = false;
				savedSuccessfully = false;
				if (path == "default.xml") {
					savingToDefaultError = true;
				}
				else {
					saveFromRoot(&root, path);
					previousPath = path;
					savedSuccessfully = true;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel")) {
				path = previousPath;
				showSaveDialog = false;
			}

			if (savedSuccessfully) {
				ImGui::Text("Saved successfully");
			}
			if (savingToDefaultError) {
				ImGui::Text("Cannot overwrite default.xml");
			}


			ImGui::End();
		}



		rlImGuiEnd();

		EndDrawing();

		if (IsKeyPressed(KEY_ESCAPE)) {
			selectedChild = -1;
		}

		// shortcut for resetting window position
		if (IsKeyDown(KEY_LEFT_CONTROL)) {
			if (IsKeyDown(KEY_LEFT_SHIFT)) {
				if (IsKeyPressed(KEY_R)) {
					initializedListWindow = false;
					initializedEditWindow = false;
				}
			}
		}
	}


	
	rlImGuiShutdown();
	CloseWindow();
}

// for /subsystem:windows
int WinMain(int argc, char** argv) {
	main(argc, argv);
}