//go:build mage

package main

import (
	"errors"
	"fmt"
	"os"
	"runtime"
	"strings"

	"github.com/magefile/mage/mg"
	"github.com/magefile/mage/sh"

	"github.com/charmbracelet/lipgloss"
)

const (
	OS_WINDOWS = 1
	OS_LINUX   = 2
	OS_MACOS   = 3

	ARCH_X64   = 1
	ARCH_ARM64 = 2
)

var (
	PROJECT_NAME   = "reload"
	TOOL_PATH      = "tools/bin/windows/amd64"
	BUILD_TOOL     = "vs2022"
	BUILD_PLATFORM = "windows"
	BUILD_ARCH     = "x86_64"

	OS   = OS_WINDOWS
	ARCH = ARCH_X64

	GOOS   = "windows"
	GOARCH = "amd64"

	COMMIT     = ""
	BRANCH     = ""
	BUILD_TIME = ""

	VERSION_H = "src/version.h"
)

const (
	// In real life situations we'd adjust the document to fit the width we've
	// detected. In the case of this example we're hardcoding the width, and
	// later using the detected width only to truncate in order to avoid jaggy
	// wrapping.
	width = 96

	columnWidth = 50
)

var (
	subtle     = lipgloss.AdaptiveColor{Light: "#D9DCCF", Dark: "#383838"}
	failSubtle = lipgloss.AdaptiveColor{Light: "#ffcc99", Dark: "#ff9966"}

	dialogBoxStyle = lipgloss.NewStyle().
			Border(lipgloss.RoundedBorder()).
			BorderForeground(lipgloss.Color("#0099FF")).
			Padding(1, 0).
			BorderTop(true).
			BorderLeft(true).
			BorderRight(true).
			BorderBottom(true)
	title = `
Reload
	`

	list = lipgloss.NewStyle().
		Border(lipgloss.NormalBorder(), false, false, false, false).
		BorderForeground(subtle).
		Height(8).
		Width(columnWidth + 1)

	listHeader = lipgloss.NewStyle().
			BorderStyle(lipgloss.NormalBorder()).
			BorderBottom(true).
			BorderForeground(subtle).
			MarginRight(2).
			Render

	listItem = lipgloss.NewStyle().PaddingLeft(2).Render

	sectionTitle = lipgloss.NewStyle().
			Border(lipgloss.RoundedBorder()).
			BorderForeground(lipgloss.Color("#0099FF")).
			Foreground(lipgloss.Color("#009900")).
			Padding(0, 1).
			BorderTop(true).
			BorderLeft(true).
			BorderRight(true).
			BorderBottom(true)

	failTitle = lipgloss.NewStyle().
			Border(lipgloss.RoundedBorder()).
			BorderForeground(lipgloss.Color("#990000")).
			Foreground(lipgloss.Color("#DD0000")).
			Padding(1, 1).
			BorderTop(true).
			BorderLeft(true).
			BorderRight(true).
			BorderBottom(true)
)

func details() error {
	greenStyle := lipgloss.NewStyle().Foreground(lipgloss.Color("#009900"))

	settingsList := list.Render(
		lipgloss.JoinVertical(lipgloss.Left,
			listHeader("Building with config"),
			listItem(fmt.Sprintf("OS: %s", greenStyle.Render(runtime.GOOS))),
			listItem(fmt.Sprintf("ARCH: %s", greenStyle.Render(runtime.GOARCH))),
			listItem(fmt.Sprintf("Commit: %s", greenStyle.Render(COMMIT))),
			listItem(fmt.Sprintf("Branch: %s", greenStyle.Render(BRANCH))),
			listItem(fmt.Sprintf("Build Time: %s", greenStyle.Render(BUILD_TIME))),
		),
	)

	centreContent := lipgloss.JoinVertical(lipgloss.Center,
		greenStyle.Render(title),
		settingsList,
	)

	centreTitle := lipgloss.NewStyle().Width(70).Align(lipgloss.Center).Render(centreContent)

	titleText := lipgloss.Place(width, 9,
		lipgloss.Center, lipgloss.Center,
		dialogBoxStyle.Render(centreTitle),
		lipgloss.WithWhitespaceChars("."),
		lipgloss.WithWhitespaceForeground(subtle),
	)

	fmt.Println(titleText)
	fmt.Println()

	return nil
}

func printSectionTitle(title string) {

	fmt.Println(
		lipgloss.Place(width, 3,
			lipgloss.Center, lipgloss.Center,
			sectionTitle.Render(title),
			lipgloss.WithWhitespaceChars("."),
			lipgloss.WithWhitespaceForeground(subtle),
		),
	)
	fmt.Println()
}

func printFailTitle(title string) error {

	fmt.Println(
		lipgloss.Place(width, 9,
			lipgloss.Center, lipgloss.Center,
			failTitle.Render(title),
			lipgloss.WithWhitespaceChars("."),
			lipgloss.WithWhitespaceForeground(failSubtle),
		),
	)
	fmt.Println()

	return errors.New(title)
}

func setup() error {

	var err error

	// Determine Environment
	switch runtime.GOOS {
	case "windows":
		OS = OS_WINDOWS
		GOOS = "windows"
	case "linux":
		OS = OS_LINUX
		GOOS = "linux"
	case "darwin":
		OS = OS_MACOS
		GOOS = "darwin"
	}

	arch_folder := "amd64"

	switch runtime.GOARCH {
	case "amd64":
		ARCH = ARCH_X64
		GOARCH = "amd64"
	case "arm64":
		ARCH = ARCH_ARM64
		GOARCH = "arm64"
		arch_folder = "arm64"
	}

	// Configure Build Tools
	switch OS {
	case OS_WINDOWS:
		TOOL_PATH = "tools/bin/windows/" + arch_folder
		BUILD_TOOL = "vs2022"
		BUILD_PLATFORM = "windows"

	case OS_LINUX:
		TOOL_PATH = "tools/bin/linux/" + arch_folder
		BUILD_TOOL = "gmake"
		BUILD_PLATFORM = "linux"

	case OS_MACOS:
		TOOL_PATH = "tools/bin/darwin/" + arch_folder
		BUILD_TOOL = "xcode4"

		switch ARCH {
		case ARCH_X64:
			BUILD_PLATFORM = "macosx"
		case ARCH_ARM64:
			BUILD_PLATFORM = "macosxARM"
			BUILD_ARCH = "arm64"
		}

	}

	// Determine Versions
	COMMIT, err = sh.Output("git", "rev-parse", "--short", "HEAD")

	if err != nil {
		return printFailTitle("Failed to determine Git Commit")
	}

	BRANCH, err = sh.Output("git", "rev-parse", "--abbrev-ref", "HEAD")
	if err != nil {
		return printFailTitle("Failed to determine Git Branch")
	}

	BUILD_TIME, err = sh.Output("date")

	if err != nil {
		return printFailTitle("Failed to determine build time")
	}

	return details()
}

type Build mg.Namespace

func (Build) All() error {
	var err error
	fmt.Println("Building All...")

	err = Project{}.Debug()

	if err != nil {
		return err
	}

	err = Module{}.Basic()

	if err != nil {
		return err
	}

	return nil
}

func (Build) Info() error {
	return setup()
}

func (Build) Gen() error {

	err := setup()

	if err != nil {
		printFailTitle("Build Setup Failed...")
		return err
	}

	printSectionTitle("Running Code Generation")

	ext := ""
	if OS == OS_WINDOWS {
		ext = ".exe"
	}

	fmt.Printf("\n\n")
	fmt.Printf("Setting Versions: Commit: %s Branch: %s\n", COMMIT, BRANCH)
	ran, err := sh.Exec(nil, os.Stdout, os.Stderr, "git", "checkout", VERSION_H)

	if !ran || err != nil {
		return printFailTitle("Failed to git checkout for version.h. Error: " + err.Error())
	}

	ran, err = sh.Exec(nil, os.Stdout, os.Stderr, TOOL_PATH+"/version"+ext, "-commit="+COMMIT, "-branch="+BRANCH, "-display=true", "-version_file=src/version.h")
	if !ran || err != nil {
		return printFailTitle("Failed to run version tool.\nError: " + err.Error())
	}

	ran, err = sh.Exec(nil, os.Stdout, os.Stderr, TOOL_PATH+"/premake5"+ext, BUILD_TOOL, "platform="+BUILD_PLATFORM)
	BUILD_PLATFORM = ""
	if !ran || err != nil {
		return printFailTitle("Failed to generate projects.\nError: " + err.Error())
	}

	printSectionTitle("Code Gen Finished")

	return nil
}

func goToolBuild(target, description string) error {

	err := Build{}.Gen()

	if err != nil {
		printFailTitle("Code Gen Failed...")
		return err
	}

	printSectionTitle(fmt.Sprintf("Building Tool: %s - %s", target, description))

	ext := ""
	if OS == OS_WINDOWS {
		ext = ".exe"
	}

	// Path to tool source code
	sourcePath := fmt.Sprintf("tools/%s/main.go", target)

	// Generate path to tool binary
	targetBinary := fmt.Sprintf("%s/%s%s", TOOL_PATH, target, ext)

	// Run the go build
	ran, err := sh.Exec(nil, os.Stdout, os.Stderr, "go", "build", "-o", targetBinary, sourcePath)

	if !ran || err != nil {
		return printFailTitle("Failed to build tool: " + target + ". Error: " + err.Error())
	}

	printSectionTitle(fmt.Sprintf("Sucessfully Built Tool @ Path: %s\n", targetBinary))

	return nil
}

func coreBuild(target, config string) error {

	err := Build{}.Gen()

	if err != nil {
		fmt.Println("Code Gen Failed...")
		return err
	}

	linuxConfig := fmt.Sprintf("%s_linux64", strings.ToLower(config))

	printSectionTitle(fmt.Sprintf("Building Target: %s", target))

	switch OS {

	case OS_WINDOWS:
		// Run the windows build

		ran, err := sh.Exec(
			nil,
			os.Stdout,
			os.Stderr,
			"msbuild.exe",
			"./projects/"+PROJECT_NAME+".sln",
			"-p:Configuration="+config,
			"-target:"+target,
		)

		if !ran || err != nil {
			return printFailTitle("Failed to build Windows target: " + target + ". Error: " + err.Error())
		}
	case OS_MACOS:
		// Run the mac build
		ran, err := sh.Exec(
			nil,
			os.Stdout,
			os.Stderr,
			"xcodebuild",
			"-project",
			"./projects/"+target+".xcodeproj",
			"-configuration",
			config,
			"ARCHS="+BUILD_ARCH,
			"-target",
			target,
			"-destination",
			"platform=macOS",
		)

		if !ran || err != nil {
			return printFailTitle("Failed to build macOS target: " + target + ". Error: " + err.Error())
		}

	case OS_LINUX:
		ran, err := sh.Exec(
			nil,
			os.Stdout,
			os.Stderr,
			"make",
			"-C",
			"projects",
			target,
			"config="+linuxConfig,
		)

		if !ran || err != nil {
			return printFailTitle("Failed to build linux target: " + target + ". Error: " + err.Error())
		}
	}

	printSectionTitle(fmt.Sprintf("Build Finished: %s", target))

	return nil

}

// Clean Targets
// -------------

func (Build) Clean_project() error {

	// Clean the Tiny Project Executables
	switch OS {
	case OS_WINDOWS:
		ran, err := sh.Exec(nil, os.Stdout, os.Stderr, "cmd", "/c", "rmdir", "/s", "/q", "build\\"+PROJECT_NAME+".exe")
		if !ran || err != nil {
			return printFailTitle("Failed to clean " + PROJECT_NAME + ".exe. Error: " + err.Error())
		}
	default:
		ran, err := sh.Exec(nil, os.Stdout, os.Stderr, "rm", "-rf", "build/"+PROJECT_NAME+"")
		if !ran || err != nil {
			return printFailTitle("Failed to clean " + PROJECT_NAME + ". Error: " + err.Error())
		}
	}

	// Clean the Tiny Project Obj files
	obj_folder := ""
	rm_flags := ""
	switch OS {
	case OS_WINDOWS:
		obj_folder = "Windows"
		rm_flags = "-r"
	case OS_LINUX:
		obj_folder = "linux64"
		rm_flags = "-Rf"
	case OS_MACOS:
		obj_folder = "macosx"
		rm_flags = "-Rf"
	}

	ran, err := sh.Exec(nil, os.Stdout, os.Stderr, "rm", rm_flags, "projects/obj/"+obj_folder+"/Debug/"+PROJECT_NAME)
	if !ran || err != nil {
		return printFailTitle("Failed to clean " + PROJECT_NAME + ". Error: " + err.Error())
	}

	ran, err = sh.Exec(nil, os.Stdout, os.Stderr, "rm", rm_flags, "projects/obj/"+obj_folder+"/Release/"+PROJECT_NAME)
	if !ran || err != nil {
		return printFailTitle("Failed to clean " + PROJECT_NAME + ". Error: " + err.Error())
	}

	return nil
}

func (Build) Clean() error {

	switch OS {
	case OS_WINDOWS:
		ran, err := sh.Exec(nil, os.Stdout, os.Stderr, "rm", "-r", "build", "projects")
		if !ran || err != nil {
			return printFailTitle("Failed to clean build and project folders. Error: " + err.Error())
		}
	default:
		ran, err := sh.Exec(nil, os.Stdout, os.Stderr, "rm", "-Rf", "build", "projects")
		if !ran || err != nil {
			return printFailTitle("Failed to clean build and project folders. Error: " + err.Error())
		}
	}

	return nil
}

// Project Build Targets
// ------------------

type Project mg.Namespace

func (Project) Debug() error {
	return coreBuild(PROJECT_NAME, "Debug")
}

func (Project) Release() error {
	return coreBuild(PROJECT_NAME, "Release")
}

// Module Build Targets
// --------------------

type Module mg.Namespace

func (Module) Basic() error {
	return coreBuild("basic", "Debug")
}

// Test Build Targets
// ------------------

type Test mg.Namespace

func (Test) Build() error {
	return coreBuild("test", "Debug")
}

// Tool Build Targets
// ------------------

type Tool mg.Namespace

func (Tool) Version() error {
	return goToolBuild("version", "version.h generator tool")
}

func (Tool) Mage() error {
	fmt.Println("Building Standalone Mage Build binaries...")

	osList := []string{
		"windows",
		"linux",
		"darwin",
	}

	archList := []string{
		"amd64",
		"arm64",
	}

	for _, osname := range osList {

		ext := ""
		if osname == "windows" {
			ext = ".exe"
		}

		for _, arch := range archList {

			targetBinary := fmt.Sprintf("../tools/%s/%s/%s%s", osname, arch, "build", ext)

			ran, err := sh.Exec(nil, os.Stdout, os.Stderr, "mage", "-compile", targetBinary, "-goarch="+arch, "-goos="+osname)
			BUILD_PLATFORM = ""
			if !ran || err != nil {
				return printFailTitle("Failed to git checkout for Version.h.\nError: " + err.Error())
			}
		}
	}

	return nil
}

func (Tool) All() error {
	var err error

	err = Tool{}.Version()

	if err != nil {
		return err
	}

	return nil
}

// Run Targets
// -----------
type Run mg.Namespace

func (Run) Project() error {
	// Run the editor
	ran, err := sh.Exec(nil, os.Stdout, os.Stderr, "./build/"+PROJECT_NAME)

	if !ran || err != nil {
		return printFailTitle("Failed to run editor. Error: " + err.Error())
	}

	return nil
}

// Misc Targets
// ------------

type Open mg.Namespace

func (Open) IDE() error {
	err := setup()

	if err != nil {
		return err
	}

	switch OS {
	case OS_WINDOWS:
		ran, err := sh.Exec(nil, os.Stdout, os.Stderr, "start", ".\\projects\\"+PROJECT_NAME+".sln")
		if !ran || err != nil {
			return printFailTitle("Failed to open the Visual Studio " + PROJECT_NAME + " project. Error: " + err.Error())
		}
	case OS_MACOS:
		ran, err := sh.Exec(nil, os.Stdout, os.Stderr, "open", "./projects/"+PROJECT_NAME+".xcworkspace")
		if !ran || err != nil {
			return printFailTitle("Failed to open the Xcode " + PROJECT_NAME + " project. Error: " + err.Error())
		}

	}

	return nil
}

func (Open) Code() error {
	ran, err := sh.Exec(nil, os.Stdout, os.Stderr, "code", ".")

	if !ran || err != nil {
		return printFailTitle("Failed to open project in VSCode. Error: " + err.Error())
	}

	return nil
}

var Default = Build.All
