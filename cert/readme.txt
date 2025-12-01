On the web side Joystick and Codecs are working in secure environment only
Thus we have to use https, but not http. The simplest way is self signed certificates.
Ideally it should be personalized on every device and signed by REEDL as a Certification 
authority (CA). Then REEDL CA certificate needs to be installed on the user device.

Another option get a well known CA signed certificates common for ?all devices? or for 
an each device (what about thousands device batch?).
