BEGIN {
    FS = ":.*##H "
}

/##H/ && $0 !~ /^[[:space:]]*#/ && $0 !~ /@awk.*##H/ {
    target = $1
    doc = $2
    category = "General"

    if (doc ~ /^@/) {
        separator = index(doc, " ")
        if (separator > 0) {
            category = substr(doc, 2, separator - 2)
            doc = substr(doc, separator + 1)
        }
    }

    if (length(target) > max)
        max = length(target)
    targets[++count] = target
    docs[count] = doc
    categories[count] = category
}

END {
    last_category = ""
    for (index = 1; index <= count; ++index) {
        if (categories[index] != last_category) {
            printf "\n\033[1;36m%s Commands:\033[0m\n", categories[index]
            last_category = categories[index]
        }
        printf "  \033[1;34m%-*s\033[0m  %s\n", max, targets[index], docs[index]
    }
    print ""
}
