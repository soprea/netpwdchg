# netpwdchg

Not ready for production but working.
<br><br>
telnet on localhost 4000 and run:
test;test123;test1234;test1234;
username;current_password;new_password;new_password;

The ";" between strings are mandatory(this is how I get the length of the string).

The order is mandatory as well.

By default it will change the password if it's older then 146650 seconds(this time is taken into consideration by every user, not just the one that you have configured in authuser(modification time of shadow vs dateNow))

The daemon wakes up every hour to check how old the shadow file is.
