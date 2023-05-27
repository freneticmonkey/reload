package main

import (
	"errors"
	"flag"
	"log"
	"os"
	"strings"
	"time"
)

func main() {
	var (
		versionFile string
		version     string
		commit      string
		branch      string
		buildTime   string
		display     bool
	)

	flag.StringVar(&versionFile, "version_file", "version.h", "path to version.h")
	flag.StringVar(&version, "version", "0.0.1", "build version of the binary")
	flag.StringVar(&commit, "commit", "<commit>", "build commit id")
	flag.StringVar(&branch, "branch", "<branch>", "build branch of the binary")
	flag.BoolVar(&display, "display", false, "display result to console")
	flag.Parse()

	buildTime = time.Now().UTC().Format(time.UnixDate)

	if display {
		log.Println("Version Values:")
		log.Printf("version_file: %s", versionFile)
		log.Printf("commit: %s", commit)
		log.Printf("branch: %s", branch)
		log.Printf("build time: %s", buildTime)
	}

	if _, err := os.Stat(versionFile); errors.Is(err, os.ErrNotExist) {
		log.Fatal("Version file does not exist")
	}

	versionContents, err := os.ReadFile(versionFile)

	if err != nil {
		log.Fatal("Failed to read Version file")
	}

	versionStr := string(versionContents)

	versionStr = strings.Replace(versionStr, "<version>", version, 1)
	versionStr = strings.Replace(versionStr, "<commit>", commit, 1)
	versionStr = strings.Replace(versionStr, "<branch>", branch, 1)
	versionStr = strings.Replace(versionStr, "<build_time>", buildTime, 1)

	if display {
		log.Printf("Version Contents:\n%s", versionStr)
	}

	os.WriteFile(versionFile, []byte(versionStr), 0644)

	log.Println("Versions Updated")
}
