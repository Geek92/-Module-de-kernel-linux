#define BUF_LEN 1024  // Taille du tampon circulaire

static ssize_t device_read(struct file *filp,
                           char __user *buffer,
                           size_t length,
                           loff_t *offset)
{
    /* Nombre d'octets réellement lus dans le tampon */
    int bytes_read = 0;

    /* Position actuelle de lecture dans le tampon circulaire */
    static int read_position = 0;

    /* Utiliser un mutex pour éviter les conflits d'accès concurrents si nécessaire */

    /* Boucle pour lire les données du tampon circulaire */
    while (length > 0) {
        char data;

        /* Lire un octet du tampon circulaire à la position actuelle */
        data = message[read_position];

        /* Si nous avons atteint la fin du message (null terminator), sortir de la boucle */
        if (data == '\0') {
            break;
        }

        /* Copier l'octet lu dans le tampon utilisateur */
        if (put_user(data, buffer) != 0) {
            /* Gestion de l'erreur de copie vers l'utilisateur si nécessaire */
            return -EFAULT;
        }

        /* Mettre à jour les compteurs et les positions */
        length--;
        buffer++;
        bytes_read++;
        read_position++;

        /* Si nous atteignons la fin du tampon circulaire, revenir au début */
        if (read_position >= BUF_LEN) {
            read_position = 0;
        }
    }

    /* Si nous avons lu au moins un octet, réinitialiser le décalage à zéro */
    if (bytes_read > 0) {
        *offset = 0;
    }

    return bytes_read;
}
