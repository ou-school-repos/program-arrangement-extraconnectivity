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
    for (i = 1; i <= count; ++i) {
        if (categories[i] != last_category) {
            printf "\n\033[1;36m%s Commands:\033[0m\n", categories[i]
            last_category = categories[i]
        }
        printf "  \033[1;34m%-*s\033[0m  %s\n", max, targets[i], docs[i]
    }
    print ""
}
