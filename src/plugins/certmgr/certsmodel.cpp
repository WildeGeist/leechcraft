/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "certsmodel.h"
#include <QDateTime>
#include <QStringList>
#include <QBrush>
#include <QApplication>
#include <QPalette>
#include <QtDebug>

namespace LC
{
namespace CertMgr
{
	QModelIndex CertsModel::index (int row, int column, const QModelIndex& parent) const
	{
		if (!hasIndex (row, column, parent))
			return {};

		if (!parent.isValid ())
			return createIndex (row, column, -1);

		if (static_cast<int> (parent.internalId ()) != -1)
			return {};

		auto id = static_cast<quint32> (parent.row ());
		return createIndex (row, column, id);
	}

	QModelIndex CertsModel::parent (const QModelIndex& child) const
	{
		if (!child.isValid ())
			return {};

		auto id = static_cast<int> (child.internalId ());
		if (id == -1)
			return {};

		return index (id, 0, {});
	}

	int CertsModel::columnCount (const QModelIndex&) const
	{
		return 1;
	}

	int CertsModel::rowCount (const QModelIndex& parent) const
	{
		if (!parent.isValid ())
			return Issuer2Certs_.size ();

		const auto id = static_cast<int> (parent.internalId ());
		return id == -1 ?
				Issuer2Certs_.value (parent.row ()).second.size () :
				0;
	}

	namespace
	{
		QString MakeTooltip (const QSslCertificate& cert)
		{
			QString result;

			auto add = [&result] (const QString& name, const QString& val)
			{
				if (val.isEmpty ())
					return;

				result += "<tr><td align='right'>" + name + ": </td>";
				result += "<td>" + val + "</td></tr>";
			};

			auto addStdFields = [&add] (auto&& listGetter)
			{
				const auto getter = [listGetter] (QSslCertificate::SubjectInfo key)
						{ return listGetter (key).join ("; "); };
				add (CertsModel::tr ("Organization"),
						getter (QSslCertificate::Organization));
				add (CertsModel::tr ("Unit"),
						getter (QSslCertificate::OrganizationalUnitName));
				add (CertsModel::tr ("Common name"),
						getter (QSslCertificate::CommonName));
				add (CertsModel::tr ("Locality"),
						getter (QSslCertificate::LocalityName));
				add (CertsModel::tr ("State"),
						getter (QSslCertificate::StateOrProvinceName));
				add (CertsModel::tr ("Country"),
						getter (QSslCertificate::CountryName));
			};

			result += "<strong>" + CertsModel::tr ("Issuee") + ":</strong>";
			result += "<table style='border: none'>";
			addStdFields ([&cert] (QSslCertificate::SubjectInfo info)
						{ return cert.subjectInfo (info); });

			add (CertsModel::tr ("Serial number"), cert.serialNumber ());
			result += "</table><br />";

			result += "<strong>" + CertsModel::tr ("Issuer") + ":</strong>";
			result += "<table style='border: none'>";
			addStdFields ([&cert] (QSslCertificate::SubjectInfo info)
						{ return cert.issuerInfo (info); });
			result += "</table><br />";

			result += "<strong>" + CertsModel::tr ("Dates") + ":</strong>";
			result += "<table style='border: none'>";
			add (CertsModel::tr ("Valid since"),
					QLocale {}.toString (cert.effectiveDate (), QLocale::ShortFormat));
			add (CertsModel::tr ("Valid until"),
					QLocale {}.toString (cert.expiryDate (), QLocale::ShortFormat));
			result += "</table><br />";

			const auto& subjs = cert.subjectAlternativeNames ();
			if (!subjs.isEmpty ())
			{
				result += "<strong>" + CertsModel::tr ("Alternate names") + ":</strong>";

				for (auto key : subjs.keys ())
				{
					QString name;
					switch (key)
					{
					case QSsl::DnsEntry:
						name = CertsModel::tr ("DNS");
						break;
					case QSsl::EmailEntry:
						name = CertsModel::tr ("Email");
						break;
					case QSsl::IpAddressEntry:
						name = CertsModel::tr ("IP address");
						break;
					}

					add (name, QStringList { subjs.values (key) }.join ("; "));
				}
			}

			return result;
		}
	}

	QVariant CertsModel::data (const QModelIndex& index, int role) const
	{
		if (!index.isValid ())
			return {};

		const auto id = static_cast<int> (index.internalId ());

		if (id == -1)
			switch (role)
			{
			case Qt::DisplayRole:
				return { Issuer2Certs_.value (index.row ()).first };
			default:
				return {};
			}

		const auto& cert = Issuer2Certs_.value (id).second.value (index.row ());

		switch (role)
		{
		case Qt::DisplayRole:
		{
			const auto& name = cert.subjectInfo (QSslCertificate::CommonName);
			const auto& org = cert.subjectInfo (QSslCertificate::Organization);

			if (!name.isEmpty () && !org.isEmpty ())
				return QString ("%1 (%2)")
						.arg (name.join ("; "))
						.arg (org.join ("; "));
			else if (!name.isEmpty ())
				return name;
			else
				return org;
		}
		case Qt::ToolTipRole:
			return MakeTooltip (cert);
		case Qt::ForegroundRole:
		{
			const auto colorGroup = Blacklisted_.contains (cert) ?
					QPalette::Disabled :
					QPalette::Normal;
			return QBrush { qApp->palette ().color (colorGroup, QPalette::Text) };
		}
		case CertificateRole:
			return QVariant::fromValue (cert);
		default:
			return {};
		}
	}

	void CertsModel::AddCert (const QSslCertificate& cert)
	{
		const auto pos = CreateListPosForCert (cert);

		const auto& parentIndex = index (std::distance (Issuer2Certs_.begin (), pos), 0, {});

		beginInsertRows (parentIndex, pos->second.size (), pos->second.size ());
		pos->second << cert;
		endInsertRows ();
	}

	void CertsModel::RemoveCert (const QSslCertificate& cert)
	{
		const auto pos = GetListPosForCert (cert);
		if (pos == Issuer2Certs_.end ())
			return;

		const auto certIdx = pos->second.indexOf (cert);
		if (certIdx == -1)
			return;

		const auto issuerRow = std::distance (Issuer2Certs_.begin (), pos);
		const auto& parentIndex = index (issuerRow, 0, {});

		beginRemoveRows (parentIndex, certIdx, certIdx);
		pos->second.removeAt (certIdx);
		endRemoveRows ();

		if (pos->second.isEmpty ())
		{
			beginRemoveRows ({}, issuerRow, issuerRow);
			Issuer2Certs_.erase (pos);
			endRemoveRows ();
		}
	}

	void CertsModel::ResetCerts (const QList<QSslCertificate>& certs)
	{
		beginResetModel ();

		Issuer2Certs_.clear ();

		for (const auto& cert : certs)
			CreateListForCert (cert) << cert;

		endResetModel ();
	}

	QModelIndex CertsModel::FindCertificate (const QSslCertificate& cert) const
	{
		const auto listPos = GetListPosForCert (cert);
		if (listPos == Issuer2Certs_.end ())
			return {};

		const auto certIdx = listPos->second.indexOf (cert);
		if (certIdx == -1)
			return {};

		const auto& parent = index (std::distance (Issuer2Certs_.begin (), listPos), 0, {});
		return index (certIdx, 0, parent);
	}

	void CertsModel::SetBlacklisted (const QSslCertificate& cert, bool blacklisted)
	{
		if (blacklisted)
			Blacklisted_ << cert;
		else
			Blacklisted_.removeAll (cert);

		const auto& idx = FindCertificate (cert);
		if (!idx.isValid ())
			return;

		emit dataChanged (idx, idx);
	}

	namespace
	{
		QString GetIssuerName (const QSslCertificate& cert)
		{
			auto issuer = cert.issuerInfo (QSslCertificate::Organization);
			if (issuer.isEmpty ())
				issuer = cert.issuerInfo (QSslCertificate::CommonName);
			return issuer.join ("; ");
		}

		template<typename T>
		auto FindIssuer2Certs (T&& vec, const QString& issuer)
		{
			const auto pos = std::lower_bound (vec.begin (), vec.end (), issuer,
					[] (const auto& item, const QString& issuer)
						{ return QString::localeAwareCompare (item.first.toLower (), issuer.toLower ()) < 0; });
			if (pos == vec.end ())
				return std::make_tuple (pos, pos);

			return pos->first.toLower () == issuer.toLower () ?
					std::make_tuple (pos, pos) :
					std::make_tuple (vec.end (), pos);
		}
	}

	auto CertsModel::GetListPosForCert (const QSslCertificate& cert) -> CertsDict_t::iterator
	{
		return std::get<0> (FindIssuer2Certs (Issuer2Certs_, GetIssuerName (cert)));
	}

	auto CertsModel::GetListPosForCert (const QSslCertificate& cert) const -> CertsDict_t::const_iterator
	{
		return std::get<0> (FindIssuer2Certs (Issuer2Certs_, GetIssuerName (cert)));
	}

	auto CertsModel::CreateListPosForCert (const QSslCertificate& cert) -> CertsDict_t::iterator
	{
		const auto& issuer = GetIssuerName (cert);

		auto [pos, hint] = FindIssuer2Certs (Issuer2Certs_, issuer);

		if (pos != Issuer2Certs_.end ())
			return pos;

		const auto row = std::distance (Issuer2Certs_.begin (), hint);
		beginInsertRows ({}, row, row);
		hint = Issuer2Certs_.insert (hint, { issuer, {} });
		endInsertRows ();

		return hint;
	}

	QList<QSslCertificate>& CertsModel::CreateListForCert (const QSslCertificate& cert)
	{
		return CreateListPosForCert (cert)->second;
	}
}
}
