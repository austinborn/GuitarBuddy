
#cd MuseData
#for folder in */
dir=$(pwd)
echo $(dir)
echo $PWD
make -C $PWD






    # if [ "${folder}" = "Austin's old data/" ]
    # then
    #     continue
    # fi

    # if [ ! -d "../CSVData/${folder}" ]
    # then
    #     mkdir "../CSVData/${folder}"
    # fi

    # cd "${folder}"

    # for file in *.muse
    # do
    #     if [ -f "../../CSVData/${folder}/${file%.*}.csv" ]
    #     then
    #         echo "${file%.*}.csv already exists!"
    #         continue
    #     fi

    #     echo "Converting ${file}..."
    #     muse-player -f "${file}" -C "../../CSVData/${folder}/${file%.*}.csv"
    # done

    # cd ../

echo "Done"