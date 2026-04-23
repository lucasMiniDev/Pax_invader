import customtkinter as ctk
import usb.core
import usb.util

VENDOR_ID = 0x3890
PRODUCT_ID = 0x0101

ctk.set_appearance_mode("dark")

class USBTool(ctk.CTk):

    def __init__(self):
        super().__init__()

        self.title("PAX USB Tool 😈")
        self.geometry("800x500")

        self.dev = None

        self.connect_btn = ctk.CTkButton(self, text="Conectar USB", command=self.connect_usb)
        self.connect_btn.pack(pady=5)

        self.input_box = ctk.CTkEntry(self, width=600, placeholder_text="HEX ou texto")
        self.input_box.pack(pady=10)

        self.send_btn = ctk.CTkButton(self, text="Enviar", command=self.send_data)
        self.send_btn.pack(pady=5)

        self.output_box = ctk.CTkTextbox(self, width=750, height=300)
        self.output_box.pack(pady=10)

    def log(self, msg):
        self.output_box.insert("end", msg + "\n")
        self.output_box.see("end")

    def connect_usb(self):
        self.dev = usb.core.find(idVendor=VENDOR_ID, idProduct=PRODUCT_ID)

        if self.dev is None:
            self.log("[ERRO] Dispositivo não encontrado")
            return

        self.dev.set_configuration()
        self.cfg = self.dev.get_active_configuration()
        self.intf = self.cfg[(0,0)]

        self.endpoint_out = usb.util.find_descriptor(
            self.intf,
            custom_match=lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_OUT
        )

        self.endpoint_in = usb.util.find_descriptor(
            self.intf,
            custom_match=lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_IN
        )

        self.log("[+] Conectado via USB")

    def send_data(self):
        if not self.dev:
            self.log("[!] Não conectado")
            return

        raw = self.input_box.get()

        try:
            data = bytes(raw, "utf-8").decode("unicode_escape").encode("latin1")
        except Exception as e:
            self.log(f"[!] erro ao converter: {e}")
            data = raw.encode("utf-8", errors="ignore")

        try:
            self.dev.write(self.endpoint_out.bEndpointAddress, data)
            self.log(f"[TX] {data}")

            try:
                response = self.dev.read(self.endpoint_in.bEndpointAddress, 64, timeout=1000)
                self.log(f"[RX] {bytes(response)}")
            except:
                self.log("[RX] timeout")

        except Exception as e:
            self.log(f"[ERRO USB] {e}")


if __name__ == "__main__":
    app = USBTool()
    app.mainloop()
