set terminal png

set grid
set datafile separator ';'

set output "u.png"
plot \
 "LEMOS_jetmixer_rm_radial_xD0.0.csv" u 1:2 w l,\
 "LEMOS_jetmixer_rm_radial_xD0.1.csv" u 1:2 w l,\
 "LEMOS_jetmixer_rm_radial_xD0.6.csv" u 1:2 w l,\
 "LEMOS_jetmixer_rm_radial_xD1.1.csv" u 1:2 w l,\
 "LEMOS_jetmixer_rm_radial_xD1.6.csv" u 1:2 w l,\
 "LEMOS_jetmixer_rm_radial_xD2.1.csv" u 1:2 w l,\
 "LEMOS_jetmixer_rm_radial_xD2.6.csv" u 1:2 w l,\
 "LEMOS_jetmixer_rm_radial_xD3.1.csv" u 1:2 w l,\
 "LEMOS_jetmixer_rm_radial_xD5.1.csv" u 1:2 w l,\
 "LEMOS_jetmixer_rm_radial_xD7.1.csv" u 1:2 w l,\
 "LEMOS_jetmixer_rm_radial_xD9.1.csv" u 1:2 w l

set output "uprime.png"
plot \
 "LEMOS_jetmixer_rm_radial_xD0.0.csv" u 1:3 w l,\
 "LEMOS_jetmixer_rm_radial_xD0.1.csv" u 1:3 w l,\
 "LEMOS_jetmixer_rm_radial_xD0.6.csv" u 1:3 w l,\
 "LEMOS_jetmixer_rm_radial_xD1.1.csv" u 1:3 w l,\
 "LEMOS_jetmixer_rm_radial_xD1.6.csv" u 1:3 w l,\
 "LEMOS_jetmixer_rm_radial_xD2.1.csv" u 1:3 w l,\
 "LEMOS_jetmixer_rm_radial_xD2.6.csv" u 1:3 w l,\
 "LEMOS_jetmixer_rm_radial_xD3.1.csv" u 1:3 w l,\
 "LEMOS_jetmixer_rm_radial_xD5.1.csv" u 1:3 w l,\
 "LEMOS_jetmixer_rm_radial_xD7.1.csv" u 1:3 w l,\
 "LEMOS_jetmixer_rm_radial_xD9.1.csv" u 1:3 w l
