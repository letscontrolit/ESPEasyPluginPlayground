Current PZEM004T plugin is not compatible with PZEM004Tv30 (modbus communication). It is also not compatible with commmunication with several PZEM004T on the same serial link.
Proposal is a new Plugin for new PZEM004Tv30: Voltage/current/power/energy/frequency/cos phi probe using serial Modbus communication protocol.
Features:

    Read values
    Erase energy value
    Set and reset address
    Access to several PZEMs simultaneously


Hardware part:
PZEM004Tv30 can be wired in parallel without any aditionnal component: simply connect RX of PZEM together and Tx of PZEM together.
However, if programming the address, only one PZEM must be connected at once during address setting.

Software part:
If you want to use several PZEM, you need to add several occurence of the pluggin.
First occurence of pluggin allows to configure serial interface and modify address of PZEM.
Next occurence of the pluggin only allows to read an adress and reset energy
Energy can also be reset with command : http://<espeasyip>/control?cmd=resetenergy,<PZEM address> (example: "http://192.168.0.1/control?cmd=resetenergy,2" => Reset energy of the PZEM with address #2)

I modify PZEM004Tv30.h and PZEM004Tv30.cpp in order to be compatible with espeay serial. Please use it rather than official PZEM004Tv30.h and PZEM004Tv30.cpp from PZEM github.

Known restriction:
Don't use HW serial when several PZEM connected in series. Over 2 PZEM, frames conflict occurs and I don't know why. The solution is to use SW serial with GPIO1 as TX and GPIO3 as RX (or to use other pins with software serial)
I try only with WEMOS D1mini.
