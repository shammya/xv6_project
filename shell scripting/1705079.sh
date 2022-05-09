function isValid(){
    local -n var-name = "sam"

}

working_directory="null"
file_name="null"
working_directory=$1
file_name=$2

if [ -z "${working_directory}" ] || [ -z "${file_name}" ] ; then # directory  and file both empty
    echo "enter a valid file name"
    read file_name
    working_directory=$PWD # working directory is pwd
else
    working_directory="$PWD""/${working_directory}"
fi



echo "working directory : ${working_directory}" 
# echo $file_name 
cd "${working_directory}"

while true ; do
    # echo  "inwhile"
    if [ -e ${file_name} ] ;then 
        break
    else
        echo "enter a valid file name"
        read  file_name
    fi
done

# taking invalid file types 
invalid=()
while IFS= read -r line; do
    # echo "Text read from file: $line"
    invalid+=("${line}")
done < "${file_name}"
echo "${invalid[@]}"

mkdir -p -- "../output"


# making directory for others
mkdir -p -- "../output/others"


# copying files into specific folders
valid_length=1
# echo $valid_length
valid=()
valid+="others"
for i in `find "${working_directory}/" -type f`
do 
    extension="${i##*.}"
    if [[ ! " ${invalid[*]} " =~ " ${extension} " ]]; then #extension does not belong to invalid array
        if [[ ${extension} == *"/"* ]] ; then
            dir="../output/others"
            mkdir -p -- ${dir}
            cp ${i} ${dir} 
            # echo ${extension}
        else
            dir="../output/""${extension}"
            mkdir -p -- ${dir}
            cp ${i} ${dir} 
            if [[ ! " ${valid[*]} " =~ " ${extension} " ]]; then #  unique extensions
                valid+=("${extension}")
                valid_length=$((valid_length+1))
            # echo ${extension}
            fi
        fi
    fi
done
valid+=("others")

echo $PWD

# echo "${valid[@]}"

# creating path files
for i in "${valid[@]}"
do
  touch "../output/""${i}""/path.txt"
done

# writing paths and count each type extension

declare -A map
for file in `find "../output" -type f`
do
    extension="${file##*.}"
    if [[  $extension == *"/"* ]]; then
        map["others"]=$((map["others"]+1))
        echo $file >> "../output/others/path.txt"
    else 
        echo $file >> "../output/$extension/path.txt"
        map[$extension]=$((map[$extension]+1))
    fi
done

# adjusting increased count
map["txt"]=$(( ${map["txt"]}-${valid_length} ))

for it in "${!map[@]}"
do 
    echo "$it, ${map[$it]}" >> "../output/count.csv"
done




# function display_value() {
#     echo "The value is $1"
# }

# amount=1
# display_value $amount
# amount=2
# display_value $amount

