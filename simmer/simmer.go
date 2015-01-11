package main

import (
	"errors"
	"flag"
	"fmt"
	"github.com/tarm/goserial"
	"io"
	"log"
	"net/http"
	"net/url"
	"os"
	"path/filepath"
	"strings"
	"sync"
	"time"
)

var errNotConnected = errors.New("no open port")

func FindUsbDevice(name string) (string, error) {
	if name != "" {
		return fmt.Sprintf("/dev/%s", name), nil
	}

	files, err := filepath.Glob("/dev/tty.usbmodem*")
	if err != nil {
		return "", err
	}

	switch len(files) {
	case 0:
		return "", errors.New("")
	case 1:
		return files[0], nil
	default:
		return "", fmt.Errorf("too many devices to choose from: [%s]", strings.Join(files, ", "))
	}
}

type Port struct {
	rw io.ReadWriteCloser
	lc sync.RWMutex
}

func setWriter(p *Port, rw io.ReadWriteCloser) {
	p.lc.Lock()
	defer p.lc.Unlock()
	p.rw = rw
}

func (p *Port) CopyTo(r io.ReadWriteCloser, w io.Writer) error {
	setWriter(p, r)
	_, err := io.Copy(w, p.rw)
	setWriter(p, nil)
	return err
}

func (p *Port) Write(b []byte) (int, error) {
	p.lc.RLock()
	defer p.lc.RUnlock()
	if p.rw == nil {
		return 0, errNotConnected
	}

	return p.rw.Write(b)
}

func waitForRetry(rc int) {
	if rc > 10 {
		time.Sleep(5 * time.Second)
	} else if rc > 5 {
		time.Sleep(1 * time.Second)
	} else if rc > 2 {
		time.Sleep(200 * time.Millisecond)
	}
}

func Monitor(p *Port, cfg *serial.Config) {
	rc := 0
	for {
		rw, err := serial.OpenPort(cfg)
		if err != nil {
			waitForRetry(rc)
			rc++
			continue
		}

		rc = 0
		p.CopyTo(rw, os.Stdout)
	}
}

func commandServer(f *flag.FlagSet, args []string) {
	flagDevc := f.String("device", "", "")
	flagAddr := f.String("addr", ":6077", "")
	if err := f.Parse(args); err != nil {
		log.Panic(err)
	}

	name, err := FindUsbDevice(*flagDevc)
	if err != nil {
		log.Panic(err)
	}

	cfg := serial.Config{
		Name: name,
		Baud: 9600,
	}

	var port Port
	go Monitor(&port, &cfg)

	h := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		log.Println("client connected.")
	})

	if err := http.ListenAndServe(*flagAddr, h); err != nil {
		log.Panic(err)
	}
}

func commandSetTemperature(f *flag.FlagSet, args []string) {
	flagAddr := f.String("addr", "localhost:6077", "")
	if err := f.Parse(args); err != nil {
		log.Panic(err)
	}

	if f.NArg() != 1 {
		fmt.Fprintln(os.Stderr, "usage")
		os.Exit(1)
	}

	uri := fmt.Sprintf("http://%s/", *flagAddr)
	res, err := http.PostForm(uri, url.Values{
		"command": {"set-temp"},
		"value":   {f.Arg(0)},
	})
	if err != nil {
		log.Panic(err)
	}
	defer res.Body.Close()

	if res.StatusCode != http.StatusOK {
		log.Panic(fmt.Errorf("status code: %d", res.StatusCode))
	}

	fmt.Println("ok")
}

func ParseArgs(args []string) (string, []string) {
	if len(args) == 0 {
		return "server", nil
	}

	return args[0], args[1:]
}

func main() {
	cmd, args := ParseArgs(os.Args[1:])
	f := flag.NewFlagSet(cmd, flag.PanicOnError)

	switch cmd {
	case "server":
		commandServer(f, args)
	case "set-temp":
		commandSetTemperature(f, args)
	default:
		log.Panic(fmt.Errorf("bad command: %s", cmd))
	}
}
