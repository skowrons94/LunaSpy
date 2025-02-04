import os
import gc
import time
import ROOT

import threading
import time

from memory_profiler import profile

import tracemalloc

#gc.enable()
#ROOT.TH1.SetDirectory(False)

#ROOT.SetOwnership(ROOT, False)

class ru_spy:
    def __init__(self, host='localhost', port=6060):
        self.host = host
        self.port = port
        self.socket = None
        self.running = False
        self.thread = None
        self.histo = ROOT.TH1F("Histogram 1", "Histogram 1", 32768, 0, 32768)
        self.msg = ROOT.TMessage( )
        self.obj = ROOT.MakeNullPointer(ROOT.TH1F)

    def connect(self):
        self.socket = ROOT.TSocket(self.host, self.port)
        if not self.socket.IsValid():
            raise ConnectionError(f"Error connecting to {self.host}:{self.port}")
        return

    def disconnect(self):
        self.socket.Close()
        del self.socket
        return

    def send(self, msg):
        if self.socket.Send(msg) <= 0: return False
        return

    @profile
    def receive(self):
        self.obj = ROOT.MakeNullPointer(ROOT.TH1F)
        nbytes = self.socket.Recv(self.msg)
        if nbytes <= 0:
            return False
        self.obj = self.msg.ReadObject(self.msg.GetClass())
        self.msg.Delete( )
        return self.obj
        
    def start(self, state):

        # Prepare the buffer
        self.buff = { "energy": [], "qshort": [], "qlong": [], "wave1": [], "wave2": [] }
        for key in self.buff.keys():
            for i in range(100):
                self.buff[key].append(ROOT.MakeNullPointer(ROOT.TH1F))
        
        self.running = True
        if( self.thread != None ): self.thread.join()
        self.thread = threading.Thread(target=self.run)
        self.thread.start()

    @profile
    def run(self):

        while self.running:
            self.collect()
            time.sleep(0.1)

    @profile
    def collect(self):
        indexes = { "energy": 0, "qshort": 0, "qlong": 0, "wave1": 0, "wave2": 0 }
        self.connect()
        self.send("get")
        try:
            while True:
                obj = self.receive()                
                if( obj == False ): break
                if "Wave1" in obj.GetName():
                    self.buff["wave1"][indexes["wave1"]] = obj
                    indexes["wave1"] += 1
                elif "Wave2" in obj.GetName():
                    self.buff["wave2"][indexes["wave2"]] = obj
                    indexes["wave2"] += 1
                elif "QShort" in obj.GetName():
                    self.buff["qshort"][indexes["qshort"]] = obj
                    indexes["qshort"] += 1
                elif "QLong" in obj.GetName():
                    self.buff["qlong"][indexes["qlong"]] = obj
                    indexes["qlong"] += 1
                elif "Energy" in obj.GetName():
                    self.buff["energy"][indexes["energy"]] = obj
                    indexes["energy"] += 1
                else:
                    obj.Delete()
        except Exception as e:
            print(f"Error receiving histogram: {e}")

        # Delete the buffer
        for key in self.buff.keys():
            for i in range(len(self.buff[key])):
                try:
                    self.buff[key][i].Delete()
                except:
                    pass

        self.disconnect()
        time.sleep(0.1)  # Wait for 1 second before next acquisition

    def stop(self):
        self.running = False
        self.thread.join()
        self.thread = None

        #try:
            #self.connect()
            #self.send("stop")
            #self.disconnect()
        #except:
            #os.system("killall RUSpy")
    
    def get_object(self, name, idx):
        try:
            return self.buff[name][idx].Clone( )
        except:
            return self.histo
        
if __name__ == "__main__":
    spy = ru_spy()
    spy.start({ "boards": [ { "name": "V1724", "dpp": "DPP-PHA", "chan": 8 } ], "run": 1 })
    time.sleep(200)
    spy.stop()
    print("Done")
    exit( )