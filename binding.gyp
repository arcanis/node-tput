{
    "targets" : [
        {
            "target_name" : "tput",
            "sources" : [ "sources/tput.cc" ],
            "libraries" : [ "-lcurses" ],
            "include_dirs": [ "<!(node -e \"require('nan')\")" ]
        }
    ]
}
